int
nouveau_channel_new(struct nouveau_drm *drm, struct nvif_device *device,
		    u32 arg0, u32 arg1, bool priv,
		    struct nouveau_channel **pchan)
{
	struct nouveau_cli *cli = (void *)device->object.client;
	bool super;
	int ret;

	/* hack until fencenv50 is fixed, and agp access relaxed */
	super = cli->base.super;
	cli->base.super = true;

	ret = nouveau_channel_ind(drm, device, arg0, priv, pchan);
	if (ret) {
		NV_PRINTK(dbg, cli, "ib channel create, %d\n", ret);
		ret = nouveau_channel_dma(drm, device, pchan);
		if (ret) {
			NV_PRINTK(dbg, cli, "dma channel create, %d\n", ret);
			goto done;
		}
	}

	ret = nouveau_channel_init(*pchan, arg0, arg1);
	if (ret) {
		NV_PRINTK(err, cli, "channel failed to initialise, %d\n", ret);
		nouveau_channel_del(pchan);
	}

	ret = nouveau_svmm_join((*pchan)->vmm->svmm, (*pchan)->inst);
	if (ret)
		nouveau_channel_del(pchan);

done:
	cli->base.super = super;
	return ret;
}
