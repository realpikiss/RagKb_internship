static void __net_exit xfrm6_tunnel_net_exit(struct net *net)
{
	struct xfrm6_tunnel_net *xfrm6_tn = xfrm6_tunnel_pernet(net);
	unsigned int i;

	xfrm_flush_gc();
	xfrm_state_flush(net, 0, false, true);

	for (i = 0; i < XFRM6_TUNNEL_SPI_BYADDR_HSIZE; i++)
		WARN_ON_ONCE(!hlist_empty(&xfrm6_tn->spi_byaddr[i]));

	for (i = 0; i < XFRM6_TUNNEL_SPI_BYSPI_HSIZE; i++)
		WARN_ON_ONCE(!hlist_empty(&xfrm6_tn->spi_byspi[i]));
}