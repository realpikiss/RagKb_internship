static vm_fault_t dmirror_devmem_fault(struct vm_fault *vmf)
{
	struct migrate_vma args;
	unsigned long src_pfns = 0;
	unsigned long dst_pfns = 0;
	struct page *rpage;
	struct dmirror *dmirror;
	vm_fault_t ret;

	/*
	 * Normally, a device would use the page->zone_device_data to point to
	 * the mirror but here we use it to hold the page for the simulated
	 * device memory and that page holds the pointer to the mirror.
	 */
	rpage = vmf->page->zone_device_data;
	dmirror = rpage->zone_device_data;

	/* FIXME demonstrate how we can adjust migrate range */
	args.vma = vmf->vma;
	args.start = vmf->address;
	args.end = args.start + PAGE_SIZE;
	args.src = &src_pfns;
	args.dst = &dst_pfns;
	args.pgmap_owner = dmirror->mdevice;
	args.flags = dmirror_select_device(dmirror);

	if (migrate_vma_setup(&args))
		return VM_FAULT_SIGBUS;

	ret = dmirror_devmem_fault_alloc_and_copy(&args, dmirror);
	if (ret)
		return ret;
	migrate_vma_pages(&args);
	/*
	 * No device finalize step is needed since
	 * dmirror_devmem_fault_alloc_and_copy() will have already
	 * invalidated the device page table.
	 */
	migrate_vma_finalize(&args);
	return 0;
}
