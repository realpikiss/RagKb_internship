static int vma_replace_policy(struct vm_area_struct *vma,
						struct mempolicy *pol)
{
	int err;
	struct mempolicy *old;
	struct mempolicy *new;

	pr_debug("vma %lx-%lx/%lx vm_ops %p vm_file %p set_policy %p\n",
		 vma->vm_start, vma->vm_end, vma->vm_pgoff,
		 vma->vm_ops, vma->vm_file,
		 vma->vm_ops ? vma->vm_ops->set_policy : NULL);

	new = mpol_dup(pol);
	if (IS_ERR(new))
		return PTR_ERR(new);

	if (vma->vm_ops && vma->vm_ops->set_policy) {
		err = vma->vm_ops->set_policy(vma, new);
		if (err)
			goto err_out;
	}

	old = vma->vm_policy;
	vma->vm_policy = new; /* protected by mmap_lock */
	mpol_put(old);

	return 0;
 err_out:
	mpol_put(new);
	return err;
}
