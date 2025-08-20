static int joydev_handle_JSIOCSAXMAP(struct joydev *joydev,
				     void __user *argp, size_t len)
{
	__u8 *abspam;
	int i;
	int retval = 0;

	len = min(len, sizeof(joydev->abspam));

	/* Validate the map. */
	abspam = memdup_user(argp, len);
	if (IS_ERR(abspam))
		return PTR_ERR(abspam);

	for (i = 0; i < joydev->nabs; i++) {
		if (abspam[i] > ABS_MAX) {
			retval = -EINVAL;
			goto out;
		}
	}

	memcpy(joydev->abspam, abspam, len);

	for (i = 0; i < joydev->nabs; i++)
		joydev->absmap[joydev->abspam[i]] = i;

 out:
	kfree(abspam);
	return retval;
}
