static int u2fzero_probe(struct hid_device *hdev,
			 const struct hid_device_id *id)
{
	struct u2fzero_device *dev;
	unsigned int minor;
	int ret;

	if (!hid_is_usb(hdev))
		return -EINVAL;

	dev = devm_kzalloc(&hdev->dev, sizeof(*dev), GFP_KERNEL);
	if (dev == NULL)
		return -ENOMEM;

	dev->hw_revision = id->driver_data;

	dev->buf_out = devm_kmalloc(&hdev->dev,
		sizeof(struct u2f_hid_report), GFP_KERNEL);
	if (dev->buf_out == NULL)
		return -ENOMEM;

	dev->buf_in = devm_kmalloc(&hdev->dev,
		sizeof(struct u2f_hid_msg), GFP_KERNEL);
	if (dev->buf_in == NULL)
		return -ENOMEM;

	ret = hid_parse(hdev);
	if (ret)
		return ret;

	dev->hdev = hdev;
	hid_set_drvdata(hdev, dev);
	mutex_init(&dev->lock);

	ret = hid_hw_start(hdev, HID_CONNECT_HIDRAW);
	if (ret)
		return ret;

	u2fzero_fill_in_urb(dev);

	dev->present = true;

	minor = ((struct hidraw *) hdev->hidraw)->minor;

	ret = u2fzero_init_led(dev, minor);
	if (ret) {
		hid_hw_stop(hdev);
		return ret;
	}

	hid_info(hdev, "%s LED initialised\n", hw_configs[dev->hw_revision].name);

	ret = u2fzero_init_hwrng(dev, minor);
	if (ret) {
		hid_hw_stop(hdev);
		return ret;
	}

	hid_info(hdev, "%s RNG initialised\n", hw_configs[dev->hw_revision].name);

	return 0;
}
