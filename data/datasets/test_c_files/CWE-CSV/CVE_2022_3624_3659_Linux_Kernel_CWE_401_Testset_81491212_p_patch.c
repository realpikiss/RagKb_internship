static struct slave *rlb_arp_xmit(struct sk_buff *skb, struct bonding *bond)
{
	struct slave *tx_slave = NULL;
	struct net_device *dev;
	struct arp_pkt *arp;

	if (!pskb_network_may_pull(skb, sizeof(*arp)))
		return NULL;
	arp = (struct arp_pkt *)skb_network_header(skb);

	/* Don't modify or load balance ARPs that do not originate locally
	 * (e.g.,arrive via a bridge).
	 */
	if (!bond_slave_has_mac_rx(bond, arp->mac_src))
		return NULL;

	dev = ip_dev_find(dev_net(bond->dev), arp->ip_src);
	if (dev) {
		if (netif_is_bridge_master(dev)) {
			dev_put(dev);
			return NULL;
		}
		dev_put(dev);
	}

	if (arp->op_code == htons(ARPOP_REPLY)) {
		/* the arp must be sent on the selected rx channel */
		tx_slave = rlb_choose_channel(skb, bond, arp);
		if (tx_slave)
			bond_hw_addr_copy(arp->mac_src, tx_slave->dev->dev_addr,
					  tx_slave->dev->addr_len);
		netdev_dbg(bond->dev, "(slave %s): Server sent ARP Reply packet\n",
			   tx_slave ? tx_slave->dev->name : "NULL");
	} else if (arp->op_code == htons(ARPOP_REQUEST)) {
		/* Create an entry in the rx_hashtbl for this client as a
		 * place holder.
		 * When the arp reply is received the entry will be updated
		 * with the correct unicast address of the client.
		 */
		tx_slave = rlb_choose_channel(skb, bond, arp);

		/* The ARP reply packets must be delayed so that
		 * they can cancel out the influence of the ARP request.
		 */
		bond->alb_info.rlb_update_delay_counter = RLB_UPDATE_DELAY;

		/* arp requests are broadcast and are sent on the primary
		 * the arp request will collapse all clients on the subnet to
		 * the primary slave. We must register these clients to be
		 * updated with their assigned mac.
		 */
		rlb_req_update_subnet_clients(bond, arp->ip_src);
		netdev_dbg(bond->dev, "(slave %s): Server sent ARP Request packet\n",
			   tx_slave ? tx_slave->dev->name : "NULL");
	}

	return tx_slave;
}
