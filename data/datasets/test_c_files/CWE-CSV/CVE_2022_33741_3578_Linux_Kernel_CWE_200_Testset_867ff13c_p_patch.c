static int talk_to_netback(struct xenbus_device *dev,
			   struct netfront_info *info)
{
	const char *message;
	struct xenbus_transaction xbt;
	int err;
	unsigned int feature_split_evtchn;
	unsigned int i = 0;
	unsigned int max_queues = 0;
	struct netfront_queue *queue = NULL;
	unsigned int num_queues = 1;
	u8 addr[ETH_ALEN];

	info->netdev->irq = 0;

	/* Check if backend is trusted. */
	info->bounce = !xennet_trusted ||
		       !xenbus_read_unsigned(dev->nodename, "trusted", 1);

	/* Check if backend supports multiple queues */
	max_queues = xenbus_read_unsigned(info->xbdev->otherend,
					  "multi-queue-max-queues", 1);
	num_queues = min(max_queues, xennet_max_queues);

	/* Check feature-split-event-channels */
	feature_split_evtchn = xenbus_read_unsigned(info->xbdev->otherend,
					"feature-split-event-channels", 0);

	/* Read mac addr. */
	err = xen_net_read_mac(dev, addr);
	if (err) {
		xenbus_dev_fatal(dev, err, "parsing %s/mac", dev->nodename);
		goto out_unlocked;
	}
	eth_hw_addr_set(info->netdev, addr);

	info->netback_has_xdp_headroom = xenbus_read_unsigned(info->xbdev->otherend,
							      "feature-xdp-headroom", 0);
	if (info->netback_has_xdp_headroom) {
		/* set the current xen-netfront xdp state */
		err = talk_to_netback_xdp(info, info->netfront_xdp_enabled ?
					  NETBACK_XDP_HEADROOM_ENABLE :
					  NETBACK_XDP_HEADROOM_DISABLE);
		if (err)
			goto out_unlocked;
	}

	rtnl_lock();
	if (info->queues)
		xennet_destroy_queues(info);

	/* For the case of a reconnect reset the "broken" indicator. */
	info->broken = false;

	err = xennet_create_queues(info, &num_queues);
	if (err < 0) {
		xenbus_dev_fatal(dev, err, "creating queues");
		kfree(info->queues);
		info->queues = NULL;
		goto out;
	}
	rtnl_unlock();

	/* Create shared ring, alloc event channel -- for each queue */
	for (i = 0; i < num_queues; ++i) {
		queue = &info->queues[i];
		err = setup_netfront(dev, queue, feature_split_evtchn);
		if (err)
			goto destroy_ring;
	}

again:
	err = xenbus_transaction_start(&xbt);
	if (err) {
		xenbus_dev_fatal(dev, err, "starting transaction");
		goto destroy_ring;
	}

	if (xenbus_exists(XBT_NIL,
			  info->xbdev->otherend, "multi-queue-max-queues")) {
		/* Write the number of queues */
		err = xenbus_printf(xbt, dev->nodename,
				    "multi-queue-num-queues", "%u", num_queues);
		if (err) {
			message = "writing multi-queue-num-queues";
			goto abort_transaction_no_dev_fatal;
		}
	}

	if (num_queues == 1) {
		err = write_queue_xenstore_keys(&info->queues[0], &xbt, 0); /* flat */
		if (err)
			goto abort_transaction_no_dev_fatal;
	} else {
		/* Write the keys for each queue */
		for (i = 0; i < num_queues; ++i) {
			queue = &info->queues[i];
			err = write_queue_xenstore_keys(queue, &xbt, 1); /* hierarchical */
			if (err)
				goto abort_transaction_no_dev_fatal;
		}
	}

	/* The remaining keys are not queue-specific */
	err = xenbus_printf(xbt, dev->nodename, "request-rx-copy", "%u",
			    1);
	if (err) {
		message = "writing request-rx-copy";
		goto abort_transaction;
	}

	err = xenbus_printf(xbt, dev->nodename, "feature-rx-notify", "%d", 1);
	if (err) {
		message = "writing feature-rx-notify";
		goto abort_transaction;
	}

	err = xenbus_printf(xbt, dev->nodename, "feature-sg", "%d", 1);
	if (err) {
		message = "writing feature-sg";
		goto abort_transaction;
	}

	err = xenbus_printf(xbt, dev->nodename, "feature-gso-tcpv4", "%d", 1);
	if (err) {
		message = "writing feature-gso-tcpv4";
		goto abort_transaction;
	}

	err = xenbus_write(xbt, dev->nodename, "feature-gso-tcpv6", "1");
	if (err) {
		message = "writing feature-gso-tcpv6";
		goto abort_transaction;
	}

	err = xenbus_write(xbt, dev->nodename, "feature-ipv6-csum-offload",
			   "1");
	if (err) {
		message = "writing feature-ipv6-csum-offload";
		goto abort_transaction;
	}

	err = xenbus_transaction_end(xbt, 0);
	if (err) {
		if (err == -EAGAIN)
			goto again;
		xenbus_dev_fatal(dev, err, "completing transaction");
		goto destroy_ring;
	}

	return 0;

 abort_transaction:
	xenbus_dev_fatal(dev, err, "%s", message);
abort_transaction_no_dev_fatal:
	xenbus_transaction_end(xbt, 1);
 destroy_ring:
	xennet_disconnect_backend(info);
	rtnl_lock();
	xennet_destroy_queues(info);
 out:
	rtnl_unlock();
out_unlocked:
	device_unregister(&dev->dev);
	return err;
}
