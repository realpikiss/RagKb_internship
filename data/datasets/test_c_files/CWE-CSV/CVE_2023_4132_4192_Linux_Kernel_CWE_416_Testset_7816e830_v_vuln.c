static void smsusb_stop_streaming(struct smsusb_device_t *dev)
{
	int i;

	for (i = 0; i < MAX_URBS; i++) {
		usb_kill_urb(&dev->surbs[i].urb);
		cancel_work_sync(&dev->surbs[i].wq);

		if (dev->surbs[i].cb) {
			smscore_putbuffer(dev->coredev, dev->surbs[i].cb);
			dev->surbs[i].cb = NULL;
		}
	}
}
