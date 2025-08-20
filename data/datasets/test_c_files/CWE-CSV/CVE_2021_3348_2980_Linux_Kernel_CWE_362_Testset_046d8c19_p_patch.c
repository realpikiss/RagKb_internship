static int nbd_add_socket(struct nbd_device *nbd, unsigned long arg,
			  bool netlink)
{
	struct nbd_config *config = nbd->config;
	struct socket *sock;
	struct nbd_sock **socks;
	struct nbd_sock *nsock;
	int err;

	sock = nbd_get_socket(nbd, arg, &err);
	if (!sock)
		return err;

	/*
	 * We need to make sure we don't get any errant requests while we're
	 * reallocating the ->socks array.
	 */
	blk_mq_freeze_queue(nbd->disk->queue);

	if (!netlink && !nbd->task_setup &&
	    !test_bit(NBD_RT_BOUND, &config->runtime_flags))
		nbd->task_setup = current;

	if (!netlink &&
	    (nbd->task_setup != current ||
	     test_bit(NBD_RT_BOUND, &config->runtime_flags))) {
		dev_err(disk_to_dev(nbd->disk),
			"Device being setup by another task");
		err = -EBUSY;
		goto put_socket;
	}

	nsock = kzalloc(sizeof(*nsock), GFP_KERNEL);
	if (!nsock) {
		err = -ENOMEM;
		goto put_socket;
	}

	socks = krealloc(config->socks, (config->num_connections + 1) *
			 sizeof(struct nbd_sock *), GFP_KERNEL);
	if (!socks) {
		kfree(nsock);
		err = -ENOMEM;
		goto put_socket;
	}

	config->socks = socks;

	nsock->fallback_index = -1;
	nsock->dead = false;
	mutex_init(&nsock->tx_lock);
	nsock->sock = sock;
	nsock->pending = NULL;
	nsock->sent = 0;
	nsock->cookie = 0;
	socks[config->num_connections++] = nsock;
	atomic_inc(&config->live_connections);
	blk_mq_unfreeze_queue(nbd->disk->queue);

	return 0;

put_socket:
	blk_mq_unfreeze_queue(nbd->disk->queue);
	sockfd_put(sock);
	return err;
}
