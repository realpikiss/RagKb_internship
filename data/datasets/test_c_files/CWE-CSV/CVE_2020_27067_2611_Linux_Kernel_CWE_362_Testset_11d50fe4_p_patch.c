static void l2tp_eth_dev_uninit(struct net_device *dev)
{
	dev_put(dev);
}
