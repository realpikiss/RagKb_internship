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

	chip->dev.class = tpm_class;
	chip->dev.class->shutdown_pre = tpm_class_shutdown;
	chip->dev.release = tpm_dev_release;
	chip->dev.parent = pdev;
	chip->dev.groups = chip->groups;

	if (chip->dev_num == 0)
		chip->dev.devt = MKDEV(MISC_MAJOR, TPM_MINOR);
	else
		chip->dev.devt = MKDEV(MAJOR(tpm_devt), chip->dev_num);

	rc = dev_set_name(&chip->dev, "tpm%d", chip->dev_num);
	if (rc)
		goto out;

	if (!pdev)
		chip->flags |= TPM_CHIP_FLAG_VIRTUAL;

	cdev_init(&chip->cdev, &tpm_fops);
	chip->cdev.owner = THIS_MODULE;

	rc = tpm2_init_space(&chip->work_space, TPM2_SPACE_BUFFER_SIZE);
	if (rc) {
		rc = -ENOMEM;
		goto out;
	}

	chip->locality = -1;
	return chip;

out:
	put_device(&chip->dev);
	return ERR_PTR(rc);
}
