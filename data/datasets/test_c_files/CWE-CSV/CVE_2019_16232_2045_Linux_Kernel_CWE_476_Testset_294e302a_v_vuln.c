static int if_sdio_probe(struct sdio_func *func,
		const struct sdio_device_id *id)
{
	struct if_sdio_card *card;
	struct lbs_private *priv;
	int ret, i;
	unsigned int model;
	struct if_sdio_packet *packet;

	for (i = 0;i < func->card->num_info;i++) {
		if (sscanf(func->card->info[i],
				"802.11 SDIO ID: %x", &model) == 1)
			break;
		if (sscanf(func->card->info[i],
				"ID: %x", &model) == 1)
			break;
		if (!strcmp(func->card->info[i], "IBIS Wireless SDIO Card")) {
			model = MODEL_8385;
			break;
		}
	}

	if (i == func->card->num_info) {
		pr_err("unable to identify card model\n");
		return -ENODEV;
	}

	card = kzalloc(sizeof(struct if_sdio_card), GFP_KERNEL);
	if (!card)
		return -ENOMEM;

	card->func = func;
	card->model = model;

	switch (card->model) {
	case MODEL_8385:
		card->scratch_reg = IF_SDIO_SCRATCH_OLD;
		break;
	case MODEL_8686:
		card->scratch_reg = IF_SDIO_SCRATCH;
		break;
	case MODEL_8688:
	default: /* for newer chipsets */
		card->scratch_reg = IF_SDIO_FW_STATUS;
		break;
	}

	spin_lock_init(&card->lock);
	card->workqueue = alloc_workqueue("libertas_sdio", WQ_MEM_RECLAIM, 0);
	INIT_WORK(&card->packet_worker, if_sdio_host_to_card_worker);
	init_waitqueue_head(&card->pwron_waitq);

	/* Check if we support this card */
	for (i = 0; i < ARRAY_SIZE(fw_table); i++) {
		if (card->model == fw_table[i].model)
			break;
	}
	if (i == ARRAY_SIZE(fw_table)) {
		pr_err("unknown card model 0x%x\n", card->model);
		ret = -ENODEV;
		goto free;
	}

	sdio_set_drvdata(func, card);

	lbs_deb_sdio("class = 0x%X, vendor = 0x%X, "
			"device = 0x%X, model = 0x%X, ioport = 0x%X\n",
			func->class, func->vendor, func->device,
			model, (unsigned)card->ioport);


	priv = lbs_add_card(card, &func->dev);
	if (IS_ERR(priv)) {
		ret = PTR_ERR(priv);
		goto free;
	}

	card->priv = priv;

	priv->card = card;
	priv->hw_host_to_card = if_sdio_host_to_card;
	priv->enter_deep_sleep = if_sdio_enter_deep_sleep;
	priv->exit_deep_sleep = if_sdio_exit_deep_sleep;
	priv->reset_deep_sleep_wakeup = if_sdio_reset_deep_sleep_wakeup;
	priv->reset_card = if_sdio_reset_card;
	priv->power_save = if_sdio_power_save;
	priv->power_restore = if_sdio_power_restore;
	priv->is_polling = !(func->card->host->caps & MMC_CAP_SDIO_IRQ);
	ret = if_sdio_power_on(card);
	if (ret)
		goto err_activate_card;

out:
	return ret;

err_activate_card:
	flush_workqueue(card->workqueue);
	lbs_remove_card(priv);
free:
	destroy_workqueue(card->workqueue);
	while (card->packets) {
		packet = card->packets;
		card->packets = card->packets->next;
		kfree(packet);
	}

	kfree(card);

	goto out;
}
