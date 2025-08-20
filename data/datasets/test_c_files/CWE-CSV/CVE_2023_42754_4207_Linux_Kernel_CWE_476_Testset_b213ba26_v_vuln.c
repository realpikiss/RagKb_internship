static void ipv4_send_dest_unreach(struct sk_buff *skb)
{
	struct ip_options opt;
	int res;

	/* Recompile ip options since IPCB may not be valid anymore.
	 * Also check we have a reasonable ipv4 header.
	 */
	if (!pskb_network_may_pull(skb, sizeof(struct iphdr)) ||
	    ip_hdr(skb)->version != 4 || ip_hdr(skb)->ihl < 5)
		return;

	memset(&opt, 0, sizeof(opt));
	if (ip_hdr(skb)->ihl > 5) {
		if (!pskb_network_may_pull(skb, ip_hdr(skb)->ihl * 4))
			return;
		opt.optlen = ip_hdr(skb)->ihl * 4 - sizeof(struct iphdr);

		rcu_read_lock();
		res = __ip_options_compile(dev_net(skb->dev), &opt, skb, NULL);
		rcu_read_unlock();

		if (res)
			return;
	}
	__icmp_send(skb, ICMP_DEST_UNREACH, ICMP_HOST_UNREACH, 0, &opt);
}
