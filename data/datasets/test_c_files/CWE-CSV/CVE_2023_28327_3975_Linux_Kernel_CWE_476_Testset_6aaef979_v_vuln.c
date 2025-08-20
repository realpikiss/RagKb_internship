static int unix_diag_get_exact(struct sk_buff *in_skb,
			       const struct nlmsghdr *nlh,
			       struct unix_diag_req *req)
{
	struct net *net = sock_net(in_skb->sk);
	unsigned int extra_len;
	struct sk_buff *rep;
	struct sock *sk;
	int err;

	err = -EINVAL;
	if (req->udiag_ino == 0)
		goto out_nosk;

	sk = unix_lookup_by_ino(net, req->udiag_ino);
	err = -ENOENT;
	if (sk == NULL)
		goto out_nosk;

	err = sock_diag_check_cookie(sk, req->udiag_cookie);
	if (err)
		goto out;

	extra_len = 256;
again:
	err = -ENOMEM;
	rep = nlmsg_new(sizeof(struct unix_diag_msg) + extra_len, GFP_KERNEL);
	if (!rep)
		goto out;

	err = sk_diag_fill(sk, rep, req, NETLINK_CB(in_skb).portid,
			   nlh->nlmsg_seq, 0, req->udiag_ino);
	if (err < 0) {
		nlmsg_free(rep);
		extra_len += 256;
		if (extra_len >= PAGE_SIZE)
			goto out;

		goto again;
	}
	err = nlmsg_unicast(net->diag_nlsk, rep, NETLINK_CB(in_skb).portid);

out:
	if (sk)
		sock_put(sk);
out_nosk:
	return err;
}
