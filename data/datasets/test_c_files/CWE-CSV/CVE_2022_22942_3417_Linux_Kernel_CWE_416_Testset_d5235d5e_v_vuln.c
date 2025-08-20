int vmw_fence_event_ioctl(struct drm_device *dev, void *data,
			  struct drm_file *file_priv)
{
	struct vmw_private *dev_priv = vmw_priv(dev);
	struct drm_vmw_fence_event_arg *arg =
		(struct drm_vmw_fence_event_arg *) data;
	struct vmw_fence_obj *fence = NULL;
	struct vmw_fpriv *vmw_fp = vmw_fpriv(file_priv);
	struct ttm_object_file *tfile = vmw_fp->tfile;
	struct drm_vmw_fence_rep __user *user_fence_rep =
		(struct drm_vmw_fence_rep __user *)(unsigned long)
		arg->fence_rep;
	uint32_t handle;
	int ret;

	/*
	 * Look up an existing fence object,
	 * and if user-space wants a new reference,
	 * add one.
	 */
	if (arg->handle) {
		struct ttm_base_object *base =
			vmw_fence_obj_lookup(tfile, arg->handle);

		if (IS_ERR(base))
			return PTR_ERR(base);

		fence = &(container_of(base, struct vmw_user_fence,
				       base)->fence);
		(void) vmw_fence_obj_reference(fence);

		if (user_fence_rep != NULL) {
			ret = ttm_ref_object_add(vmw_fp->tfile, base,
						 NULL, false);
			if (unlikely(ret != 0)) {
				DRM_ERROR("Failed to reference a fence "
					  "object.\n");
				goto out_no_ref_obj;
			}
			handle = base->handle;
		}
		ttm_base_object_unref(&base);
	}

	/*
	 * Create a new fence object.
	 */
	if (!fence) {
		ret = vmw_execbuf_fence_commands(file_priv, dev_priv,
						 &fence,
						 (user_fence_rep) ?
						 &handle : NULL);
		if (unlikely(ret != 0)) {
			DRM_ERROR("Fence event failed to create fence.\n");
			return ret;
		}
	}

	BUG_ON(fence == NULL);

	ret = vmw_event_fence_action_create(file_priv, fence,
					    arg->flags,
					    arg->user_data,
					    true);
	if (unlikely(ret != 0)) {
		if (ret != -ERESTARTSYS)
			DRM_ERROR("Failed to attach event to fence.\n");
		goto out_no_create;
	}

	vmw_execbuf_copy_fence_user(dev_priv, vmw_fp, 0, user_fence_rep, fence,
				    handle, -1, NULL);
	vmw_fence_obj_unreference(&fence);
	return 0;
out_no_create:
	if (user_fence_rep != NULL)
		ttm_ref_object_base_unref(tfile, handle);
out_no_ref_obj:
	vmw_fence_obj_unreference(&fence);
	return ret;
}
