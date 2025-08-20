void kvmppc_uvmem_drop_pages(const struct kvm_memory_slot *slot,
			     struct kvm *kvm, bool skip_page_out)
{
	int i;
	struct kvmppc_uvmem_page_pvt *pvt;
	struct page *uvmem_page;
	struct vm_area_struct *vma = NULL;
	unsigned long uvmem_pfn, gfn;
	unsigned long addr;

	mmap_read_lock(kvm->mm);

	addr = slot->userspace_addr;

	gfn = slot->base_gfn;
	for (i = slot->npages; i; --i, ++gfn, addr += PAGE_SIZE) {

		/* Fetch the VMA if addr is not in the latest fetched one */
		if (!vma || addr >= vma->vm_end) {
			vma = vma_lookup(kvm->mm, addr);
			if (!vma) {
				pr_err("Can't find VMA for gfn:0x%lx\n", gfn);
				break;
			}
		}

		mutex_lock(&kvm->arch.uvmem_lock);

		if (kvmppc_gfn_is_uvmem_pfn(gfn, kvm, &uvmem_pfn)) {
			uvmem_page = pfn_to_page(uvmem_pfn);
			pvt = uvmem_page->zone_device_data;
			pvt->skip_page_out = skip_page_out;
			pvt->remove_gfn = true;

			if (__kvmppc_svm_page_out(vma, addr, addr + PAGE_SIZE,
						  PAGE_SHIFT, kvm, pvt->gpa))
				pr_err("Can't page out gpa:0x%lx addr:0x%lx\n",
				       pvt->gpa, addr);
		} else {
			/* Remove the shared flag if any */
			kvmppc_gfn_remove(gfn, kvm);
		}

		mutex_unlock(&kvm->arch.uvmem_lock);
	}

	mmap_read_unlock(kvm->mm);
}
