static int ravb_close(struct net_device *ndev)
{
	struct device_node *np = ndev->dev.parent->of_node;
	struct ravb_private *priv = netdev_priv(ndev);
	const struct ravb_hw_info *info = priv->info;
	struct ravb_tstamp_skb *ts_skb, *ts_skb2;

	netif_tx_stop_all_queues(ndev);

	/* Disable interrupts by clearing the interrupt masks. */
	ravb_write(ndev, 0, RIC0);
	ravb_write(ndev, 0, RIC2);
	ravb_write(ndev, 0, TIC);

	/* Stop PTP Clock driver */
	if (info->gptp)
		ravb_ptp_stop(ndev);

	/* Set the config mode to stop the AVB-DMAC's processes */
	if (ravb_stop_dma(ndev) < 0)
		netdev_err(ndev,
			   "device will be stopped after h/w processes are done.\n");

	/* Clear the timestamp list */
	if (info->gptp || info->ccc_gac) {
		list_for_each_entry_safe(ts_skb, ts_skb2, &priv->ts_skb_list, list) {
			list_del(&ts_skb->list);
			kfree_skb(ts_skb->skb);
			kfree(ts_skb);
		}
	}

	/* PHY disconnect */
	if (ndev->phydev) {
		phy_stop(ndev->phydev);
		phy_disconnect(ndev->phydev);
		if (of_phy_is_fixed_link(np))
			of_phy_deregister_fixed_link(np);
	}

	if (info->multi_irqs) {
		free_irq(priv->tx_irqs[RAVB_NC], ndev);
		free_irq(priv->rx_irqs[RAVB_NC], ndev);
		free_irq(priv->tx_irqs[RAVB_BE], ndev);
		free_irq(priv->rx_irqs[RAVB_BE], ndev);
		free_irq(priv->emac_irq, ndev);
		if (info->err_mgmt_irqs) {
			free_irq(priv->erra_irq, ndev);
			free_irq(priv->mgmta_irq, ndev);
		}
	}
	free_irq(ndev->irq, ndev);

	if (info->nc_queues)
		napi_disable(&priv->napi[RAVB_NC]);
	napi_disable(&priv->napi[RAVB_BE]);

	/* Free all the skb's in the RX queue and the DMA buffers. */
	ravb_ring_free(ndev, RAVB_BE);
	if (info->nc_queues)
		ravb_ring_free(ndev, RAVB_NC);

	return 0;
}
