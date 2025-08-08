static int xennet_connect(struct net_device *dev)
{
	struct netfront_info *np = netdev_priv(dev);
	unsigned int num_queues = 0;
	int err;
	unsigned int j = 0;
	struct netfront_queue *queue = NULL;

	if (!xenbus_read_unsigned(np->xbdev->otherend, "feature-rx-copy", 0)) {
		dev_info(&dev->dev,
			 "backend does not support copying receive path\n");
		return -ENODEV;
	}

	err = talk_to_netback(np->xbdev, np);
	if (err)
		return err;
	if (np->netback_has_xdp_headroom)
		pr_info("backend supports XDP headroom\n");
	if (np->bounce)
		dev_info(&np->xbdev->dev,
			 "bouncing transmitted data to zeroed pages\n");

	/* talk_to_netback() sets the correct number of queues */
	num_queues = dev->real_num_tx_queues;

	if (dev->reg_state == NETREG_UNINITIALIZED) {
		err = register_netdev(dev);
		if (err) {
			pr_warn("%s: register_netdev err=%d\n", __func__, err);
			device_unregister(&np->xbdev->dev);
			return err;
		}
	}

	rtnl_lock();
	netdev_update_features(dev);
	rtnl_unlock();

	/*
	 * All public and private state should now be sane.  Get
	 * ready to start sending and receiving packets and give the driver
	 * domain a kick because we've probably just requeued some
	 * packets.
	 */
	netif_tx_lock_bh(np->netdev);
	netif_device_attach(np->netdev);
	netif_tx_unlock_bh(np->netdev);

	netif_carrier_on(np->netdev);
	for (j = 0; j < num_queues; ++j) {
		queue = &np->queues[j];

		notify_remote_via_irq(queue->tx_irq);
		if (queue->tx_irq != queue->rx_irq)
			notify_remote_via_irq(queue->rx_irq);

		spin_lock_irq(&queue->tx_lock);
		xennet_tx_buf_gc(queue);
		spin_unlock_irq(&queue->tx_lock);

		spin_lock_bh(&queue->rx_lock);
		xennet_alloc_rx_buffers(queue);
		spin_unlock_bh(&queue->rx_lock);
	}

	return 0;
}