static int xgene_hwmon_remove(struct platform_device *pdev)
{
	struct xgene_hwmon_dev *ctx = platform_get_drvdata(pdev);

	hwmon_device_unregister(ctx->hwmon_dev);
	kfifo_free(&ctx->async_msg_fifo);
	if (acpi_disabled)
		mbox_free_channel(ctx->mbox_chan);
	else
		pcc_mbox_free_channel(ctx->pcc_chan);

	return 0;
}
