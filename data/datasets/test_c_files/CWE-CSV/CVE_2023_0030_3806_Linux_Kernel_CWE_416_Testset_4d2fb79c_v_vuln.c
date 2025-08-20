static void
nvkm_vmm_put_region(struct nvkm_vmm *vmm, struct nvkm_vma *vma)
{
	struct nvkm_vma *prev, *next;

	if ((prev = node(vma, prev)) && !prev->used) {
		rb_erase(&prev->tree, &vmm->free);
		list_del(&prev->head);
		vma->addr  = prev->addr;
		vma->size += prev->size;
		kfree(prev);
	}

	if ((next = node(vma, next)) && !next->used) {
		rb_erase(&next->tree, &vmm->free);
		list_del(&next->head);
		vma->size += next->size;
		kfree(next);
	}

	nvkm_vmm_free_insert(vmm, vma);
}
