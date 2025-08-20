static int mon_bin_mmap(struct file *filp, struct vm_area_struct *vma)
{
	/* don't do anything here: "fault" will set up page table entries */
	vma->vm_ops = &mon_bin_vm_ops;

	if (vma->vm_flags & VM_WRITE)
		return -EPERM;

	vma->vm_flags &= ~VM_MAYWRITE;
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
	vma->vm_private_data = filp->private_data;
	mon_bin_vma_open(vma);
	return 0;
}
