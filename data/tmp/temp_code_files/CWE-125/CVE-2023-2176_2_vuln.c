static int resolve_prepare_src(struct rdma_id_private *id_priv,
			       struct sockaddr *src_addr,
			       const struct sockaddr *dst_addr)
{
	int ret;

	memcpy(cma_dst_addr(id_priv), dst_addr, rdma_addr_size(dst_addr));
	if (!cma_comp_exch(id_priv, RDMA_CM_ADDR_BOUND, RDMA_CM_ADDR_QUERY)) {
		/* For a well behaved ULP state will be RDMA_CM_IDLE */
		ret = cma_bind_addr(&id_priv->id, src_addr, dst_addr);
		if (ret)
			goto err_dst;
		if (WARN_ON(!cma_comp_exch(id_priv, RDMA_CM_ADDR_BOUND,
					   RDMA_CM_ADDR_QUERY))) {
			ret = -EINVAL;
			goto err_dst;
		}
	}

	if (cma_family(id_priv) != dst_addr->sa_family) {
		ret = -EINVAL;
		goto err_state;
	}
	return 0;

err_state:
	cma_comp_exch(id_priv, RDMA_CM_ADDR_QUERY, RDMA_CM_ADDR_BOUND);
err_dst:
	memset(cma_dst_addr(id_priv), 0, rdma_addr_size(dst_addr));
	return ret;
}