static int pppol2tp_connect(struct socket *sock, struct sockaddr *uservaddr,
			    int sockaddr_len, int flags)
{
	struct sock *sk = sock->sk;
	struct sockaddr_pppol2tp *sp = (struct sockaddr_pppol2tp *) uservaddr;
	struct pppox_sock *po = pppox_sk(sk);
	struct l2tp_session *session = NULL;
	struct l2tp_tunnel *tunnel;
	struct pppol2tp_session *ps;
	struct l2tp_session_cfg cfg = { 0, };
	int error = 0;
	u32 tunnel_id, peer_tunnel_id;
	u32 session_id, peer_session_id;
	bool drop_refcnt = false;
	bool drop_tunnel = false;
	int ver = 2;
	int fd;

	lock_sock(sk);

	error = -EINVAL;
	if (sp->sa_protocol != PX_PROTO_OL2TP)
		goto end;

	/* Check for already bound sockets */
	error = -EBUSY;
	if (sk->sk_state & PPPOX_CONNECTED)
		goto end;

	/* We don't supporting rebinding anyway */
	error = -EALREADY;
	if (sk->sk_user_data)
		goto end; /* socket is already attached */

	/* Get params from socket address. Handle L2TPv2 and L2TPv3.
	 * This is nasty because there are different sockaddr_pppol2tp
	 * structs for L2TPv2, L2TPv3, over IPv4 and IPv6. We use
	 * the sockaddr size to determine which structure the caller
	 * is using.
	 */
	peer_tunnel_id = 0;
	if (sockaddr_len == sizeof(struct sockaddr_pppol2tp)) {
		fd = sp->pppol2tp.fd;
		tunnel_id = sp->pppol2tp.s_tunnel;
		peer_tunnel_id = sp->pppol2tp.d_tunnel;
		session_id = sp->pppol2tp.s_session;
		peer_session_id = sp->pppol2tp.d_session;
	} else if (sockaddr_len == sizeof(struct sockaddr_pppol2tpv3)) {
		struct sockaddr_pppol2tpv3 *sp3 =
			(struct sockaddr_pppol2tpv3 *) sp;
		ver = 3;
		fd = sp3->pppol2tp.fd;
		tunnel_id = sp3->pppol2tp.s_tunnel;
		peer_tunnel_id = sp3->pppol2tp.d_tunnel;
		session_id = sp3->pppol2tp.s_session;
		peer_session_id = sp3->pppol2tp.d_session;
	} else if (sockaddr_len == sizeof(struct sockaddr_pppol2tpin6)) {
		struct sockaddr_pppol2tpin6 *sp6 =
			(struct sockaddr_pppol2tpin6 *) sp;
		fd = sp6->pppol2tp.fd;
		tunnel_id = sp6->pppol2tp.s_tunnel;
		peer_tunnel_id = sp6->pppol2tp.d_tunnel;
		session_id = sp6->pppol2tp.s_session;
		peer_session_id = sp6->pppol2tp.d_session;
	} else if (sockaddr_len == sizeof(struct sockaddr_pppol2tpv3in6)) {
		struct sockaddr_pppol2tpv3in6 *sp6 =
			(struct sockaddr_pppol2tpv3in6 *) sp;
		ver = 3;
		fd = sp6->pppol2tp.fd;
		tunnel_id = sp6->pppol2tp.s_tunnel;
		peer_tunnel_id = sp6->pppol2tp.d_tunnel;
		session_id = sp6->pppol2tp.s_session;
		peer_session_id = sp6->pppol2tp.d_session;
	} else {
		error = -EINVAL;
		goto end; /* bad socket address */
	}

	/* Don't bind if tunnel_id is 0 */
	error = -EINVAL;
	if (tunnel_id == 0)
		goto end;

	tunnel = l2tp_tunnel_get(sock_net(sk), tunnel_id);
	if (tunnel)
		drop_tunnel = true;

	/* Special case: create tunnel context if session_id and
	 * peer_session_id is 0. Otherwise look up tunnel using supplied
	 * tunnel id.
	 */
	if ((session_id == 0) && (peer_session_id == 0)) {
		if (tunnel == NULL) {
			struct l2tp_tunnel_cfg tcfg = {
				.encap = L2TP_ENCAPTYPE_UDP,
				.debug = 0,
			};
			error = l2tp_tunnel_create(sock_net(sk), fd, ver, tunnel_id, peer_tunnel_id, &tcfg, &tunnel);
			if (error < 0)
				goto end;
		}
	} else {
		/* Error if we can't find the tunnel */
		error = -ENOENT;
		if (tunnel == NULL)
			goto end;

		/* Error if socket is not prepped */
		if (tunnel->sock == NULL)
			goto end;
	}

	if (tunnel->recv_payload_hook == NULL)
		tunnel->recv_payload_hook = pppol2tp_recv_payload_hook;

	if (tunnel->peer_tunnel_id == 0)
		tunnel->peer_tunnel_id = peer_tunnel_id;

	session = l2tp_session_get(sock_net(sk), tunnel, session_id);
	if (session) {
		drop_refcnt = true;
		ps = l2tp_session_priv(session);

		/* Using a pre-existing session is fine as long as it hasn't
		 * been connected yet.
		 */
		mutex_lock(&ps->sk_lock);
		if (rcu_dereference_protected(ps->sk,
					      lockdep_is_held(&ps->sk_lock))) {
			mutex_unlock(&ps->sk_lock);
			error = -EEXIST;
			goto end;
		}
	} else {
		/* Default MTU must allow space for UDP/L2TP/PPP headers */
		cfg.mtu = 1500 - PPPOL2TP_HEADER_OVERHEAD;
		cfg.mru = cfg.mtu;

		session = l2tp_session_create(sizeof(struct pppol2tp_session),
					      tunnel, session_id,
					      peer_session_id, &cfg);
		if (IS_ERR(session)) {
			error = PTR_ERR(session);
			goto end;
		}

		pppol2tp_session_init(session);
		ps = l2tp_session_priv(session);
		l2tp_session_inc_refcount(session);

		mutex_lock(&ps->sk_lock);
		error = l2tp_session_register(session, tunnel);
		if (error < 0) {
			mutex_unlock(&ps->sk_lock);
			kfree(session);
			goto end;
		}
		drop_refcnt = true;
	}

	/* Special case: if source & dest session_id == 0x0000, this
	 * socket is being created to manage the tunnel. Just set up
	 * the internal context for use by ioctl() and sockopt()
	 * handlers.
	 */
	if ((session->session_id == 0) &&
	    (session->peer_session_id == 0)) {
		error = 0;
		goto out_no_ppp;
	}

	/* The only header we need to worry about is the L2TP
	 * header. This size is different depending on whether
	 * sequence numbers are enabled for the data channel.
	 */
	po->chan.hdrlen = PPPOL2TP_L2TP_HDR_SIZE_NOSEQ;

	po->chan.private = sk;
	po->chan.ops	 = &pppol2tp_chan_ops;
	po->chan.mtu	 = session->mtu;

	error = ppp_register_net_channel(sock_net(sk), &po->chan);
	if (error) {
		mutex_unlock(&ps->sk_lock);
		goto end;
	}

out_no_ppp:
	/* This is how we get the session context from the socket. */
	sk->sk_user_data = session;
	rcu_assign_pointer(ps->sk, sk);
	mutex_unlock(&ps->sk_lock);

	/* Keep the reference we've grabbed on the session: sk doesn't expect
	 * the session to disappear. pppol2tp_session_destruct() is responsible
	 * for dropping it.
	 */
	drop_refcnt = false;

	sk->sk_state = PPPOX_CONNECTED;
	l2tp_info(session, L2TP_MSG_CONTROL, "%s: created\n",
		  session->name);

end:
	if (drop_refcnt)
		l2tp_session_dec_refcount(session);
	if (drop_tunnel)
		l2tp_tunnel_dec_refcount(tunnel);
	release_sock(sk);

	return error;
}
