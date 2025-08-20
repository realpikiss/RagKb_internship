static int vmw_execbuf_tie_context(struct vmw_private *dev_priv,
				   struct vmw_sw_context *sw_context,
				   uint32_t handle)
{
	struct vmw_resource *res;
	int ret;
	unsigned int size;

	if (handle == SVGA3D_INVALID_ID)
		return 0;

	size = vmw_execbuf_res_size(dev_priv, vmw_res_dx_context);
	ret = vmw_validation_preload_res(sw_context->ctx, size);
	if (ret)
		return ret;

	ret = vmw_user_resource_lookup_handle
		(dev_priv, sw_context->fp->tfile, handle,
		 user_context_converter, &res);
	if (ret != 0) {
		VMW_DEBUG_USER("Could not find or user DX context 0x%08x.\n",
			       (unsigned int) handle);
		return ret;
	}

	ret = vmw_execbuf_res_val_add(sw_context, res, VMW_RES_DIRTY_SET,
				      vmw_val_add_flag_none);
	if (unlikely(ret != 0)) {
		vmw_resource_unreference(&res);
		return ret;
	}

	sw_context->dx_ctx_node = vmw_execbuf_info_from_res(sw_context, res);
	sw_context->man = vmw_context_res_man(res);

	vmw_resource_unreference(&res);
	return 0;
}
