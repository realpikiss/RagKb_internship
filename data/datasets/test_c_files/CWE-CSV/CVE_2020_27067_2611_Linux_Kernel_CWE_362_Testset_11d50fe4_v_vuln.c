static void l2tp_eth_dev_uninit(struct net_device *dev)
{
	struct l2tp_eth *priv = netdev_priv(dev);
	struct l2tp_eth_net *pn = l2tp_eth_pernet(dev_net(dev));

	spin_lock(&pn->l2tp_eth_lock);
	list_del_init(&priv->list);
	spin_unlock(&pn->l2tp_eth_lock);
	dev_put(dev);
}
