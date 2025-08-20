struct tee_shm *tee_shm_alloc(struct tee_context *ctx, size_t size, u32 flags)
{
	struct tee_device *teedev = ctx->teedev;
	struct tee_shm_pool_mgr *poolm = NULL;
	struct tee_shm *shm;
	void *ret;
	int rc;

	if (!(flags & TEE_SHM_MAPPED)) {
		dev_err(teedev->dev.parent,
			"only mapped allocations supported\n");
		return ERR_PTR(-EINVAL);
	}

	if ((flags & ~(TEE_SHM_MAPPED | TEE_SHM_DMA_BUF | TEE_SHM_PRIV))) {
		dev_err(teedev->dev.parent, "invalid shm flags 0x%x", flags);
		return ERR_PTR(-EINVAL);
	}

	if (!tee_device_get(teedev))
		return ERR_PTR(-EINVAL);

	if (!teedev->pool) {
		/* teedev has been detached from driver */
		ret = ERR_PTR(-EINVAL);
		goto err_dev_put;
	}

	shm = kzalloc(sizeof(*shm), GFP_KERNEL);
	if (!shm) {
		ret = ERR_PTR(-ENOMEM);
		goto err_dev_put;
	}

	refcount_set(&shm->refcount, 1);
	shm->flags = flags | TEE_SHM_POOL;
	shm->ctx = ctx;
	if (flags & TEE_SHM_DMA_BUF)
		poolm = teedev->pool->dma_buf_mgr;
	else
		poolm = teedev->pool->private_mgr;

	rc = poolm->ops->alloc(poolm, shm, size);
	if (rc) {
		ret = ERR_PTR(rc);
		goto err_kfree;
	}

	if (flags & TEE_SHM_DMA_BUF) {
		mutex_lock(&teedev->mutex);
		shm->id = idr_alloc(&teedev->idr, shm, 1, 0, GFP_KERNEL);
		mutex_unlock(&teedev->mutex);
		if (shm->id < 0) {
			ret = ERR_PTR(shm->id);
			goto err_pool_free;
		}
	}

	teedev_ctx_get(ctx);

	return shm;
err_pool_free:
	poolm->ops->free(poolm, shm);
err_kfree:
	kfree(shm);
err_dev_put:
	tee_device_put(teedev);
	return ret;
}
