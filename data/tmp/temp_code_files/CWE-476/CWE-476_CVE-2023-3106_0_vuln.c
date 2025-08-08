static int xfrm_dump_sa_done(struct netlink_callback *cb)
{
	struct xfrm_state_walk *walk = (struct xfrm_state_walk *) &cb->args[1];
	struct sock *sk = cb->skb->sk;
	struct net *net = sock_net(sk);

	xfrm_state_walk_done(walk, net);
	return 0;
}