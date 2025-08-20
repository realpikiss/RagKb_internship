struct tpm_chip *tpm_chip_alloc(struct device *pdev,
				const struct tpm_class_ops *ops)
{
	struct tpm_chip *chip;
	int rc;

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (chip == NULL)
		return ERR_PTR(-ENOMEM);

	mutex_init(&chip->tpm_mutex);
	init_rwsem(&chip->ops_sem);

	chip->ops = ops;

	mutex_lock(&idr_lock);
	rc = idr_alloc(&dev_nums_idr, NULL, 0, TPM_NUM_DEVICES, GFP_KERNEL);
	mutex_unlock(&idr_lock);
	if (rc < 0) {
		dev_err(pdev, "No available tpm device numbers\n");
		kfree(chip);
		return ERR_PTR(rc);
	}
	chip->dev_num = rc;

	device_initialize(&chip->dev);
	device_initialize(&chip->devs);

	chip->dev.class = tpm_class;
	chip->dev.class->shutdown_pre = tpm_class_shutdown;
	chip->dev.release = tpm_dev_release;
	chip->dev.parent = pdev;
	chip->dev.groups = chip->groups;

	chip->devs.parent = pdev;
	chip->devs.class = tpmrm_class;
	chip->devs.release = tpm_devs_release;
	/* get extra reference on main device to hold on
	 * behalf of devs.  This holds the chip structure
	 * while cdevs is in use.  The corresponding put
	 * is in the tpm_devs_release (TPM2 only)
	 */
	if (chip->flags & TPM_CHIP_FLAG_TPM2)
		get_device(&chip->dev);

	if (chip->dev_num == 0)
		chip->dev.devt = MKDEV(MISC_MAJOR, TPM_MINOR);
	else
		chip->dev.devt = MKDEV(MAJOR(tpm_devt), chip->dev_num);

	chip->devs.devt =
		MKDEV(MAJOR(tpm_devt), chip->dev_num + TPM_NUM_DEVICES);

	rc = dev_set_name(&chip->dev, "tpm%d", chip->dev_num);
	if (rc)
		goto out;
	rc = dev_set_name(&chip->devs, "tpmrm%d", chip->dev_num);
	if (rc)
		goto out;

	if (!pdev)
		chip->flags |= TPM_CHIP_FLAG_VIRTUAL;

	cdev_init(&chip->cdev, &tpm_fops);
	cdev_init(&chip->cdevs, &tpmrm_fops);
	chip->cdev.owner = THIS_MODULE;
	chip->cdevs.owner = THIS_MODULE;

	rc = tpm2_init_space(&chip->work_space, TPM2_SPACE_BUFFER_SIZE);
	if (rc) {
		rc = -ENOMEM;
		goto out;
	}

	chip->locality = -1;
	return chip;

out:
	put_device(&chip->devs);
	put_device(&chip->dev);
	return ERR_PTR(rc);
}
