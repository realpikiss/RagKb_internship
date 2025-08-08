static int nr_listen(struct socket *sock, int backlog)
{
	struct sock *sk = sock->sk;

	lock_sock(sk);
	if (sk->sk_state != TCP_LISTEN) {
		memset(&nr_sk(sk)->user_addr, 0, AX25_ADDR_LEN);
		sk->sk_max_ack_backlog = backlog;
		sk->sk_state           = TCP_LISTEN;
		release_sock(sk);
		return 0;
	}
	release_sock(sk);

	return -EOPNOTSUPP;
}