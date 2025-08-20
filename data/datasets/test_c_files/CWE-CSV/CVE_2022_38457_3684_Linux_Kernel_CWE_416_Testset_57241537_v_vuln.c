static int vmw_cmd_dx_set_streamoutput(struct vmw_private *dev_priv,
				       struct vmw_sw_context *sw_context,
				       SVGA3dCmdHeader *header)
{
	struct vmw_ctx_validation_info *ctx_node = sw_context->dx_ctx_node;
	struct vmw_resource *res;
	struct vmw_ctx_bindinfo_so binding;
	struct {
		SVGA3dCmdHeader header;
		SVGA3dCmdDXSetStreamOutput body;
	} *cmd = container_of(header, typeof(*cmd), header);
	int ret;

	if (!ctx_node) {
		DRM_ERROR("DX Context not set.\n");
		return -EINVAL;
	}

	if (cmd->body.soid == SVGA3D_INVALID_ID)
		return 0;

	/*
	 * When device does not support SM5 then streamoutput with mob command is
	 * not available to user-space. Simply return in this case.
	 */
	if (!has_sm5_context(dev_priv))
		return 0;

	/*
	 * With SM5 capable device if lookup fails then user-space probably used
	 * old streamoutput define command. Return without an error.
	 */
	res = vmw_dx_streamoutput_lookup(vmw_context_res_man(ctx_node->ctx),
					 cmd->body.soid);
	if (IS_ERR(res)) {
		return 0;
	}

	ret = vmw_execbuf_res_noctx_val_add(sw_context, res,
					    VMW_RES_DIRTY_NONE);
	if (ret) {
		DRM_ERROR("Error creating resource validation node.\n");
		return ret;
	}

	binding.bi.ctx = ctx_node->ctx;
	binding.bi.res = res;
	binding.bi.bt = vmw_ctx_binding_so;
	binding.slot = 0; /* Only one SO set to context at a time. */

	vmw_binding_add(sw_context->dx_ctx_node->staged, &binding.bi, 0,
			binding.slot);

	return ret;
}
