int vmw_surface_define_ioctl(struct drm_device *dev, void *data,
			     struct drm_file *file_priv)
{
	struct vmw_private *dev_priv = vmw_priv(dev);
	struct vmw_user_surface *user_srf;
	struct vmw_surface *srf;
	struct vmw_surface_metadata *metadata;
	struct vmw_resource *res;
	struct vmw_resource *tmp;
	union drm_vmw_surface_create_arg *arg =
	    (union drm_vmw_surface_create_arg *)data;
	struct drm_vmw_surface_create_req *req = &arg->req;
	struct drm_vmw_surface_arg *rep = &arg->rep;
	struct ttm_object_file *tfile = vmw_fpriv(file_priv)->tfile;
	int ret;
	int i, j;
	uint32_t cur_bo_offset;
	struct drm_vmw_size *cur_size;
	struct vmw_surface_offset *cur_offset;
	uint32_t num_sizes;
	const SVGA3dSurfaceDesc *desc;

	num_sizes = 0;
	for (i = 0; i < DRM_VMW_MAX_SURFACE_FACES; ++i) {
		if (req->mip_levels[i] > DRM_VMW_MAX_MIP_LEVELS)
			return -EINVAL;
		num_sizes += req->mip_levels[i];
	}

	if (num_sizes > DRM_VMW_MAX_SURFACE_FACES * DRM_VMW_MAX_MIP_LEVELS ||
	    num_sizes == 0)
		return -EINVAL;

	desc = vmw_surface_get_desc(req->format);
	if (unlikely(desc->blockDesc == SVGA3DBLOCKDESC_NONE)) {
		VMW_DEBUG_USER("Invalid format %d for surface creation.\n",
			       req->format);
		return -EINVAL;
	}

	user_srf = kzalloc(sizeof(*user_srf), GFP_KERNEL);
	if (unlikely(!user_srf)) {
		ret = -ENOMEM;
		goto out_unlock;
	}

	srf = &user_srf->srf;
	metadata = &srf->metadata;
	res = &srf->res;

	/* Driver internally stores as 64-bit flags */
	metadata->flags = (SVGA3dSurfaceAllFlags)req->flags;
	metadata->format = req->format;
	metadata->scanout = req->scanout;

	memcpy(metadata->mip_levels, req->mip_levels,
	       sizeof(metadata->mip_levels));
	metadata->num_sizes = num_sizes;
	metadata->sizes =
		memdup_user((struct drm_vmw_size __user *)(unsigned long)
			    req->size_addr,
			    sizeof(*metadata->sizes) * metadata->num_sizes);
	if (IS_ERR(metadata->sizes)) {
		ret = PTR_ERR(metadata->sizes);
		goto out_no_sizes;
	}
	srf->offsets = kmalloc_array(metadata->num_sizes, sizeof(*srf->offsets),
				     GFP_KERNEL);
	if (unlikely(!srf->offsets)) {
		ret = -ENOMEM;
		goto out_no_offsets;
	}

	metadata->base_size = *srf->metadata.sizes;
	metadata->autogen_filter = SVGA3D_TEX_FILTER_NONE;
	metadata->multisample_count = 0;
	metadata->multisample_pattern = SVGA3D_MS_PATTERN_NONE;
	metadata->quality_level = SVGA3D_MS_QUALITY_NONE;

	cur_bo_offset = 0;
	cur_offset = srf->offsets;
	cur_size = metadata->sizes;

	for (i = 0; i < DRM_VMW_MAX_SURFACE_FACES; ++i) {
		for (j = 0; j < metadata->mip_levels[i]; ++j) {
			uint32_t stride = vmw_surface_calculate_pitch(
						  desc, cur_size);

			cur_offset->face = i;
			cur_offset->mip = j;
			cur_offset->bo_offset = cur_bo_offset;
			cur_bo_offset += vmw_surface_get_image_buffer_size
				(desc, cur_size, stride);
			++cur_offset;
			++cur_size;
		}
	}
	res->guest_memory_size = cur_bo_offset;
	if (metadata->scanout &&
	    metadata->num_sizes == 1 &&
	    metadata->sizes[0].width == VMW_CURSOR_SNOOP_WIDTH &&
	    metadata->sizes[0].height == VMW_CURSOR_SNOOP_HEIGHT &&
	    metadata->format == VMW_CURSOR_SNOOP_FORMAT) {
		const struct SVGA3dSurfaceDesc *desc =
			vmw_surface_get_desc(VMW_CURSOR_SNOOP_FORMAT);
		const u32 cursor_size_bytes = VMW_CURSOR_SNOOP_WIDTH *
					      VMW_CURSOR_SNOOP_HEIGHT *
					      desc->pitchBytesPerBlock;
		srf->snooper.image = kzalloc(cursor_size_bytes, GFP_KERNEL);
		if (!srf->snooper.image) {
			DRM_ERROR("Failed to allocate cursor_image\n");
			ret = -ENOMEM;
			goto out_no_copy;
		}
	} else {
		srf->snooper.image = NULL;
	}

	user_srf->prime.base.shareable = false;
	user_srf->prime.base.tfile = NULL;
	if (drm_is_primary_client(file_priv))
		user_srf->master = drm_file_get_master(file_priv);

	/**
	 * From this point, the generic resource management functions
	 * destroy the object on failure.
	 */

	ret = vmw_surface_init(dev_priv, srf, vmw_user_surface_free);
	if (unlikely(ret != 0))
		goto out_unlock;

	/*
	 * A gb-aware client referencing a shared surface will
	 * expect a backup buffer to be present.
	 */
	if (dev_priv->has_mob && req->shareable) {
		struct vmw_bo_params params = {
			.domain = VMW_BO_DOMAIN_SYS,
			.busy_domain = VMW_BO_DOMAIN_SYS,
			.bo_type = ttm_bo_type_device,
			.size = res->guest_memory_size,
			.pin = false
		};

		ret = vmw_gem_object_create(dev_priv,
					    &params,
					    &res->guest_memory_bo);
		if (unlikely(ret != 0)) {
			vmw_resource_unreference(&res);
			goto out_unlock;
		}
	}

	tmp = vmw_resource_reference(&srf->res);
	ret = ttm_prime_object_init(tfile, res->guest_memory_size, &user_srf->prime,
				    req->shareable, VMW_RES_SURFACE,
				    &vmw_user_surface_base_release);

	if (unlikely(ret != 0)) {
		vmw_resource_unreference(&tmp);
		vmw_resource_unreference(&res);
		goto out_unlock;
	}

	rep->sid = user_srf->prime.base.handle;
	vmw_resource_unreference(&res);

	return 0;
out_no_copy:
	kfree(srf->offsets);
out_no_offsets:
	kfree(metadata->sizes);
out_no_sizes:
	ttm_prime_object_kfree(user_srf, prime);
out_unlock:
	return ret;
}
