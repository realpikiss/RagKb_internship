static void
nvkm_vmm_put_region(struct nvkm_vmm *vmm, struct nvkm_vma *vma)
{
	struct nvkm_vma *prev, *next;

	if ((prev = node(vma, prev)) && !prev->used) {
		vma->addr  = prev->addr;
		vma->size += prev->size;
		nvkm_vmm_free_delete(vmm, prev);
	}

	if ((next = node(vma, next)) && !next->used) {
		vma->size += next->size;
		nvkm_vmm_free_delete(vmm, next);
	}

	nvkm_vmm_free_insert(vmm, vma);
}
