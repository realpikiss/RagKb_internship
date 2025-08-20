static int lme2510_identify_state(struct dvb_usb_device *d, const char **name)
{
	struct lme2510_state *st = d->priv;
	int status;

	usb_reset_configuration(d->udev);

	usb_set_interface(d->udev,
		d->props->bInterfaceNumber, 1);

	st->dvb_usb_lme2510_firmware = dvb_usb_lme2510_firmware;

	status = lme2510_return_status(d);
	if (status == 0x44) {
		*name = lme_firmware_switch(d, 0);
		return COLD;
	}

	if (status != 0x47)
		return -EINVAL;

	return WARM;
}
