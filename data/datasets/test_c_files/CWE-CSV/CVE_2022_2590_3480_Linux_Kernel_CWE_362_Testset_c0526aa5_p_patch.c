static int faultin_page(struct vm_area_struct *vma,
		unsigned long address, unsigned int *flags, bool unshare,
		int *locked)
{
	unsigned int fault_flags = 0;
	vm_fault_t ret;

	if (*flags & FOLL_NOFAULT)
		return -EFAULT;
	if (*flags & FOLL_WRITE)
		fault_flags |= FAULT_FLAG_WRITE;
	if (*flags & FOLL_REMOTE)
		fault_flags |= FAULT_FLAG_REMOTE;
	if (locked)
		fault_flags |= FAULT_FLAG_ALLOW_RETRY | FAULT_FLAG_KILLABLE;
	if (*flags & FOLL_NOWAIT)
		fault_flags |= FAULT_FLAG_ALLOW_RETRY | FAULT_FLAG_RETRY_NOWAIT;
	if (*flags & FOLL_TRIED) {
		/*
		 * Note: FAULT_FLAG_ALLOW_RETRY and FAULT_FLAG_TRIED
		 * can co-exist
		 */
		fault_flags |= FAULT_FLAG_TRIED;
	}
	if (unshare) {
		fault_flags |= FAULT_FLAG_UNSHARE;
		/* FAULT_FLAG_WRITE and FAULT_FLAG_UNSHARE are incompatible */
		VM_BUG_ON(fault_flags & FAULT_FLAG_WRITE);
	}

	ret = handle_mm_fault(vma, address, fault_flags, NULL);

	if (ret & VM_FAULT_COMPLETED) {
		/*
		 * With FAULT_FLAG_RETRY_NOWAIT we'll never release the
		 * mmap lock in the page fault handler. Sanity check this.
		 */
		WARN_ON_ONCE(fault_flags & FAULT_FLAG_RETRY_NOWAIT);
		if (locked)
			*locked = 0;
		/*
		 * We should do the same as VM_FAULT_RETRY, but let's not
		 * return -EBUSY since that's not reflecting the reality of
		 * what has happened - we've just fully completed a page
		 * fault, with the mmap lock released.  Use -EAGAIN to show
		 * that we want to take the mmap lock _again_.
		 */
		return -EAGAIN;
	}

	if (ret & VM_FAULT_ERROR) {
		int err = vm_fault_to_errno(ret, *flags);

		if (err)
			return err;
		BUG();
	}

	if (ret & VM_FAULT_RETRY) {
		if (locked && !(fault_flags & FAULT_FLAG_RETRY_NOWAIT))
			*locked = 0;
		return -EBUSY;
	}

	return 0;
}
