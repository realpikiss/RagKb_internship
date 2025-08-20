int vmw_dumb_create(struct drm_file *file_priv,
		    struct drm_device *dev,
		    struct drm_mode_create_dumb *args)
{
	struct vmw_private *dev_priv = vmw_priv(dev);
	struct vmw_bo *vbo;
	int cpp = DIV_ROUND_UP(args->bpp, 8);
	int ret;

	switch (cpp) {
	case 1: /* DRM_FORMAT_C8 */
	case 2: /* DRM_FORMAT_RGB565 */
	case 4: /* DRM_FORMAT_XRGB8888 */
		break;
	default:
		/*
		 * Dumb buffers don't allow anything else.
		 * This is tested via IGT's dumb_buffers
		 */
		return -EINVAL;
	}

	args->pitch = args->width * cpp;
	args->size = ALIGN(args->pitch * args->height, PAGE_SIZE);

	ret = vmw_gem_object_create_with_handle(dev_priv, file_priv,
						args->size, &args->handle,
						&vbo);

	return ret;
}
