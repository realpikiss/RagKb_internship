static int talk_to_blkback(struct xenbus_device *dev,
			   struct blkfront_info *info)
{
	const char *message = NULL;
	struct xenbus_transaction xbt;
	int err;
	unsigned int i, max_page_order;
	unsigned int ring_page_order;
	struct blkfront_ring_info *rinfo;

	if (!info)
		return -ENODEV;

	/* Check if backend is trusted. */
	info->bounce = !xen_blkif_trusted ||
		       !xenbus_read_unsigned(dev->nodename, "trusted", 1);

	max_page_order = xenbus_read_unsigned(info->xbdev->otherend,
					      "max-ring-page-order", 0);
	ring_page_order = min(xen_blkif_max_ring_order, max_page_order);
	info->nr_ring_pages = 1 << ring_page_order;

	err = negotiate_mq(info);
	if (err)
		goto destroy_blkring;

	for_each_rinfo(info, rinfo, i) {
		/* Create shared ring, alloc event channel. */
		err = setup_blkring(dev, rinfo);
		if (err)
			goto destroy_blkring;
	}

again:
	err = xenbus_transaction_start(&xbt);
	if (err) {
		xenbus_dev_fatal(dev, err, "starting transaction");
		goto destroy_blkring;
	}

	if (info->nr_ring_pages > 1) {
		err = xenbus_printf(xbt, dev->nodename, "ring-page-order", "%u",
				    ring_page_order);
		if (err) {
			message = "writing ring-page-order";
			goto abort_transaction;
		}
	}

	/* We already got the number of queues/rings in _probe */
	if (info->nr_rings == 1) {
		err = write_per_ring_nodes(xbt, info->rinfo, dev->nodename);
		if (err)
			goto destroy_blkring;
	} else {
		char *path;
		size_t pathsize;

		err = xenbus_printf(xbt, dev->nodename, "multi-queue-num-queues", "%u",
				    info->nr_rings);
		if (err) {
			message = "writing multi-queue-num-queues";
			goto abort_transaction;
		}

		pathsize = strlen(dev->nodename) + QUEUE_NAME_LEN;
		path = kmalloc(pathsize, GFP_KERNEL);
		if (!path) {
			err = -ENOMEM;
			message = "ENOMEM while writing ring references";
			goto abort_transaction;
		}

		for_each_rinfo(info, rinfo, i) {
			memset(path, 0, pathsize);
			snprintf(path, pathsize, "%s/queue-%u", dev->nodename, i);
			err = write_per_ring_nodes(xbt, rinfo, path);
			if (err) {
				kfree(path);
				goto destroy_blkring;
			}
		}
		kfree(path);
	}
	err = xenbus_printf(xbt, dev->nodename, "protocol", "%s",
			    XEN_IO_PROTO_ABI_NATIVE);
	if (err) {
		message = "writing protocol";
		goto abort_transaction;
	}
	err = xenbus_printf(xbt, dev->nodename, "feature-persistent", "%u",
			info->feature_persistent);
	if (err)
		dev_warn(&dev->dev,
			 "writing persistent grants feature to xenbus");

	err = xenbus_transaction_end(xbt, 0);
	if (err) {
		if (err == -EAGAIN)
			goto again;
		xenbus_dev_fatal(dev, err, "completing transaction");
		goto destroy_blkring;
	}

	for_each_rinfo(info, rinfo, i) {
		unsigned int j;

		for (j = 0; j < BLK_RING_SIZE(info); j++)
			rinfo->shadow[j].req.u.rw.id = j + 1;
		rinfo->shadow[BLK_RING_SIZE(info)-1].req.u.rw.id = 0x0fffffff;
	}
	xenbus_switch_state(dev, XenbusStateInitialised);

	return 0;

 abort_transaction:
	xenbus_transaction_end(xbt, 1);
	if (message)
		xenbus_dev_fatal(dev, err, "%s", message);
 destroy_blkring:
	blkif_free(info, 0);
	return err;
}
