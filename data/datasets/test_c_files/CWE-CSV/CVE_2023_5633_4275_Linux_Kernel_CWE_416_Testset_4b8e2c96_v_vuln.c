static int vmw_create_bo_proxy(struct drm_device *dev,
			       const struct drm_mode_fb_cmd2 *mode_cmd,
			       struct vmw_bo *bo_mob,
			       struct vmw_surface **srf_out)
{
	struct vmw_surface_metadata metadata = {0};
	uint32_t format;
	struct vmw_resource *res;
	unsigned int bytes_pp;
	int ret;

	switch (mode_cmd->pixel_format) {
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XRGB8888:
		format = SVGA3D_X8R8G8B8;
		bytes_pp = 4;
		break;

	case DRM_FORMAT_RGB565:
	case DRM_FORMAT_XRGB1555:
		format = SVGA3D_R5G6B5;
		bytes_pp = 2;
		break;

	case 8:
		format = SVGA3D_P8;
		bytes_pp = 1;
		break;

	default:
		DRM_ERROR("Invalid framebuffer format %p4cc\n",
			  &mode_cmd->pixel_format);
		return -EINVAL;
	}

	metadata.format = format;
	metadata.mip_levels[0] = 1;
	metadata.num_sizes = 1;
	metadata.base_size.width = mode_cmd->pitches[0] / bytes_pp;
	metadata.base_size.height =  mode_cmd->height;
	metadata.base_size.depth = 1;
	metadata.scanout = true;

	ret = vmw_gb_surface_define(vmw_priv(dev), &metadata, srf_out);
	if (ret) {
		DRM_ERROR("Failed to allocate proxy content buffer\n");
		return ret;
	}

	res = &(*srf_out)->res;

	/* Reserve and switch the backing mob. */
	mutex_lock(&res->dev_priv->cmdbuf_mutex);
	(void) vmw_resource_reserve(res, false, true);
	vmw_bo_unreference(&res->guest_memory_bo);
	res->guest_memory_bo = vmw_bo_reference(bo_mob);
	res->guest_memory_offset = 0;
	vmw_resource_unreserve(res, false, false, false, NULL, 0);
	mutex_unlock(&res->dev_priv->cmdbuf_mutex);

	return 0;
}
