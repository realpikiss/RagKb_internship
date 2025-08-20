int vmw_gem_object_create_ioctl(struct drm_device *dev, void *data,
				struct drm_file *filp)
{
	struct vmw_private *dev_priv = vmw_priv(dev);
	union drm_vmw_alloc_dmabuf_arg *arg =
	    (union drm_vmw_alloc_dmabuf_arg *)data;
	struct drm_vmw_alloc_dmabuf_req *req = &arg->req;
	struct drm_vmw_dmabuf_rep *rep = &arg->rep;
	struct vmw_bo *vbo;
	uint32_t handle;
	int ret;

	ret = vmw_gem_object_create_with_handle(dev_priv, filp,
						req->size, &handle, &vbo);
	if (ret)
		goto out_no_bo;

	rep->handle = handle;
	rep->map_handle = drm_vma_node_offset_addr(&vbo->tbo.base.vma_node);
	rep->cur_gmr_id = handle;
	rep->cur_gmr_offset = 0;
out_no_bo:
	return ret;
}
