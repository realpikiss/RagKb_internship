static int unix_stream_read_generic(struct unix_stream_read_state *state,
				    bool freezable)
{
	struct scm_cookie scm;
	struct socket *sock = state->socket;
	struct sock *sk = sock->sk;
	struct unix_sock *u = unix_sk(sk);
	int copied = 0;
	int flags = state->flags;
	int noblock = flags & MSG_DONTWAIT;
	bool check_creds = false;
	int target;
	int err = 0;
	long timeo;
	int skip;
	size_t size = state->size;
	unsigned int last_len;

	if (unlikely(sk->sk_state != TCP_ESTABLISHED)) {
		err = -EINVAL;
		goto out;
	}

	if (unlikely(flags & MSG_OOB)) {
		err = -EOPNOTSUPP;
		goto out;
	}

	target = sock_rcvlowat(sk, flags & MSG_WAITALL, size);
	timeo = sock_rcvtimeo(sk, noblock);

	memset(&scm, 0, sizeof(scm));

	/* Lock the socket to prevent queue disordering
	 * while sleeps in memcpy_tomsg
	 */
	mutex_lock(&u->iolock);

	skip = max(sk_peek_offset(sk, flags), 0);

	do {
		int chunk;
		bool drop_skb;
		struct sk_buff *skb, *last;

redo:
		unix_state_lock(sk);
		if (sock_flag(sk, SOCK_DEAD)) {
			err = -ECONNRESET;
			goto unlock;
		}
		last = skb = skb_peek(&sk->sk_receive_queue);
		last_len = last ? last->len : 0;
again:
		if (skb == NULL) {
			if (copied >= target)
				goto unlock;

			/*
			 *	POSIX 1003.1g mandates this order.
			 */

			err = sock_error(sk);
			if (err)
				goto unlock;
			if (sk->sk_shutdown & RCV_SHUTDOWN)
				goto unlock;

			unix_state_unlock(sk);
			if (!timeo) {
				err = -EAGAIN;
				break;
			}

			mutex_unlock(&u->iolock);

			timeo = unix_stream_data_wait(sk, timeo, last,
						      last_len, freezable);

			if (signal_pending(current)) {
				err = sock_intr_errno(timeo);
				scm_destroy(&scm);
				goto out;
			}

			mutex_lock(&u->iolock);
			goto redo;
unlock:
			unix_state_unlock(sk);
			break;
		}

		while (skip >= unix_skb_len(skb)) {
			skip -= unix_skb_len(skb);
			last = skb;
			last_len = skb->len;
			skb = skb_peek_next(skb, &sk->sk_receive_queue);
			if (!skb)
				goto again;
		}

		unix_state_unlock(sk);

		if (check_creds) {
			/* Never glue messages from different writers */
			if (!unix_skb_scm_eq(skb, &scm))
				break;
		} else if (test_bit(SOCK_PASSCRED, &sock->flags)) {
			/* Copy credentials */
			scm_set_cred(&scm, UNIXCB(skb).pid, UNIXCB(skb).uid, UNIXCB(skb).gid);
			unix_set_secdata(&scm, skb);
			check_creds = true;
		}

		/* Copy address just once */
		if (state->msg && state->msg->msg_name) {
			DECLARE_SOCKADDR(struct sockaddr_un *, sunaddr,
					 state->msg->msg_name);
			unix_copy_addr(state->msg, skb->sk);
			sunaddr = NULL;
		}

		chunk = min_t(unsigned int, unix_skb_len(skb) - skip, size);
		skb_get(skb);
		chunk = state->recv_actor(skb, skip, chunk, state);
		drop_skb = !unix_skb_len(skb);
		/* skb is only safe to use if !drop_skb */
		consume_skb(skb);
		if (chunk < 0) {
			if (copied == 0)
				copied = -EFAULT;
			break;
		}
		copied += chunk;
		size -= chunk;

		if (drop_skb) {
			/* the skb was touched by a concurrent reader;
			 * we should not expect anything from this skb
			 * anymore and assume it invalid - we can be
			 * sure it was dropped from the socket queue
			 *
			 * let's report a short read
			 */
			err = 0;
			break;
		}

		/* Mark read part of skb as used */
		if (!(flags & MSG_PEEK)) {
			UNIXCB(skb).consumed += chunk;

			sk_peek_offset_bwd(sk, chunk);

			if (UNIXCB(skb).fp) {
				scm_stat_del(sk, skb);
				unix_detach_fds(&scm, skb);
			}

			if (unix_skb_len(skb))
				break;

			skb_unlink(skb, &sk->sk_receive_queue);
			consume_skb(skb);

			if (scm.fp)
				break;
		} else {
			/* It is questionable, see note in unix_dgram_recvmsg.
			 */
			if (UNIXCB(skb).fp)
				unix_peek_fds(&scm, skb);

			sk_peek_offset_fwd(sk, chunk);

			if (UNIXCB(skb).fp)
				break;

			skip = 0;
			last = skb;
			last_len = skb->len;
			unix_state_lock(sk);
			skb = skb_peek_next(skb, &sk->sk_receive_queue);
			if (skb)
				goto again;
			unix_state_unlock(sk);
			break;
		}
	} while (size);

	mutex_unlock(&u->iolock);
	if (state->msg)
		scm_recv(sock, state->msg, &scm, flags);
	else
		scm_destroy(&scm);
out:
	return copied ? : err;
}
