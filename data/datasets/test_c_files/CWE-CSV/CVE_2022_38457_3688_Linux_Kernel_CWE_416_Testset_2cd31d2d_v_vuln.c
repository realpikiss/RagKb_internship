static int vmw_view_res_val_add(struct vmw_sw_context *sw_context,
				struct vmw_resource *view)
{
	int ret;

	/*
	 * First add the resource the view is pointing to, otherwise it may be
	 * swapped out when the view is validated.
	 */
	ret = vmw_execbuf_res_noctx_val_add(sw_context, vmw_view_srf(view),
					    vmw_view_dirtying(view));
	if (ret)
		return ret;

	return vmw_execbuf_res_noctx_val_add(sw_context, view,
					     VMW_RES_DIRTY_NONE);
}
