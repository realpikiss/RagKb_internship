static struct usb_function *gprinter_alloc(struct usb_function_instance *fi)
{
	struct printer_dev	*dev;
	struct f_printer_opts	*opts;

	opts = container_of(fi, struct f_printer_opts, func_inst);

	mutex_lock(&opts->lock);
	if (opts->minor >= minors) {
		mutex_unlock(&opts->lock);
		return ERR_PTR(-ENOENT);
	}

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		mutex_unlock(&opts->lock);
		return ERR_PTR(-ENOMEM);
	}

	kref_init(&dev->kref);
	++opts->refcnt;
	dev->minor = opts->minor;
	dev->pnp_string = opts->pnp_string;
	dev->q_len = opts->q_len;
	mutex_unlock(&opts->lock);

	dev->function.name = "printer";
	dev->function.bind = printer_func_bind;
	dev->function.setup = printer_func_setup;
	dev->function.unbind = printer_func_unbind;
	dev->function.set_alt = printer_func_set_alt;
	dev->function.disable = printer_func_disable;
	dev->function.req_match = gprinter_req_match;
	dev->function.free_func = gprinter_free;

	INIT_LIST_HEAD(&dev->tx_reqs);
	INIT_LIST_HEAD(&dev->rx_reqs);
	INIT_LIST_HEAD(&dev->rx_buffers);
	INIT_LIST_HEAD(&dev->tx_reqs_active);
	INIT_LIST_HEAD(&dev->rx_reqs_active);

	spin_lock_init(&dev->lock);
	mutex_init(&dev->lock_printer_io);
	init_waitqueue_head(&dev->rx_wait);
	init_waitqueue_head(&dev->tx_wait);
	init_waitqueue_head(&dev->tx_flush_wait);

	dev->interface = -1;
	dev->printer_cdev_open = 0;
	dev->printer_status = PRINTER_NOT_ERROR;
	dev->current_rx_req = NULL;
	dev->current_rx_bytes = 0;
	dev->current_rx_buf = NULL;

	return &dev->function;
}