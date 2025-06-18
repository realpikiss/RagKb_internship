static netdev_tx_t xennet_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct netfront_info *np = netdev_priv(dev);
	struct netfront_stats *tx_stats = this_cpu_ptr(np->tx_stats);
	struct xen_netif_tx_request *first_tx;
	unsigned int i;
	int notify;
	int slots;
	struct page *page;
	unsigned int offset;
	unsigned int len;
	unsigned long flags;
	struct netfront_queue *queue = NULL;
	struct xennet_gnttab_make_txreq info = { };
	unsigned int num_queues = dev->real_num_tx_queues;
	u16 queue_index;
	struct sk_buff *nskb;

	/* Drop the packet if no queues are set up */
	if (num_queues < 1)
		goto drop;
	if (unlikely(np->broken))
		goto drop;
	/* Determine which queue to transmit this SKB on */
	queue_index = skb_get_queue_mapping(skb);
	queue = &np->queues[queue_index];

	/* If skb->len is too big for wire format, drop skb and alert
	 * user about misconfiguration.
	 */
	if (unlikely(skb->len > XEN_NETIF_MAX_TX_SIZE)) {
		net_alert_ratelimited(
			"xennet: skb->len = %u, too big for wire format\n",
			skb->len);
		goto drop;
	}

	slots = xennet_count_skb_slots(skb);
	if (unlikely(slots > MAX_XEN_SKB_FRAGS + 1)) {
		net_dbg_ratelimited("xennet: skb rides the rocket: %d slots, %d bytes\n",
				    slots, skb->len);
		if (skb_linearize(skb))
			goto drop;
	}

	page = virt_to_page(skb->data);
	offset = offset_in_page(skb->data);

	/* The first req should be at least ETH_HLEN size or the packet will be
	 * dropped by netback.
	 *
	 * If the backend is not trusted bounce all data to zeroed pages to
	 * avoid exposing contiguous data on the granted page not belonging to
	 * the skb.
	 */
	if (np->bounce || unlikely(PAGE_SIZE - offset < ETH_HLEN)) {
		nskb = bounce_skb(skb);
		if (!nskb)
			goto drop;
		dev_consume_skb_any(skb);
		skb = nskb;
		page = virt_to_page(skb->data);
		offset = offset_in_page(skb->data);
	}

	len = skb_headlen(skb);

	spin_lock_irqsave(&queue->tx_lock, flags);

	if (unlikely(!netif_carrier_ok(dev) ||
		     (slots > 1 && !xennet_can_sg(dev)) ||
		     netif_needs_gso(skb, netif_skb_features(skb)))) {
		spin_unlock_irqrestore(&queue->tx_lock, flags);
		goto drop;
	}

	/* First request for the linear area. */
	info.queue = queue;
	info.skb = skb;
	info.page = page;
	first_tx = xennet_make_first_txreq(&info, offset, len);
	offset += info.tx_local.size;
	if (offset == PAGE_SIZE) {
		page++;
		offset = 0;
	}
	len -= info.tx_local.size;

	if (skb->ip_summed == CHECKSUM_PARTIAL)
		/* local packet? */
		first_tx->flags |= XEN_NETTXF_csum_blank |
				   XEN_NETTXF_data_validated;
	else if (skb->ip_summed == CHECKSUM_UNNECESSARY)
		/* remote but checksummed. */
		first_tx->flags |= XEN_NETTXF_data_validated;

	/* Optional extra info after the first request. */
	if (skb_shinfo(skb)->gso_size) {
		struct xen_netif_extra_info *gso;

		gso = (struct xen_netif_extra_info *)
			RING_GET_REQUEST(&queue->tx, queue->tx.req_prod_pvt++);

		first_tx->flags |= XEN_NETTXF_extra_info;

		gso->u.gso.size = skb_shinfo(skb)->gso_size;
		gso->u.gso.type = (skb_shinfo(skb)->gso_type & SKB_GSO_TCPV6) ?
			XEN_NETIF_GSO_TYPE_TCPV6 :
			XEN_NETIF_GSO_TYPE_TCPV4;
		gso->u.gso.pad = 0;
		gso->u.gso.features = 0;

		gso->type = XEN_NETIF_EXTRA_TYPE_GSO;
		gso->flags = 0;
	}

	/* Requests for the rest of the linear area. */
	xennet_make_txreqs(&info, page, offset, len);

	/* Requests for all the frags. */
	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		skb_frag_t *frag = &skb_shinfo(skb)->frags[i];
		xennet_make_txreqs(&info, skb_frag_page(frag),
					skb_frag_off(frag),
					skb_frag_size(frag));
	}

	/* First request has the packet length. */
	first_tx->size = skb->len;

	/* timestamp packet in software */
	skb_tx_timestamp(skb);

	xennet_mark_tx_pending(queue);

	RING_PUSH_REQUESTS_AND_CHECK_NOTIFY(&queue->tx, notify);
	if (notify)
		notify_remote_via_irq(queue->tx_irq);

	u64_stats_update_begin(&tx_stats->syncp);
	tx_stats->bytes += skb->len;
	tx_stats->packets++;
	u64_stats_update_end(&tx_stats->syncp);

	/* Note: It is not safe to access skb after xennet_tx_buf_gc()! */
	xennet_tx_buf_gc(queue);

	if (!netfront_tx_slot_available(queue))
		netif_tx_stop_queue(netdev_get_tx_queue(dev, queue->id));

	spin_unlock_irqrestore(&queue->tx_lock, flags);

	return NETDEV_TX_OK;

 drop:
	dev->stats.tx_dropped++;
	dev_kfree_skb_any(skb);
	return NETDEV_TX_OK;
}