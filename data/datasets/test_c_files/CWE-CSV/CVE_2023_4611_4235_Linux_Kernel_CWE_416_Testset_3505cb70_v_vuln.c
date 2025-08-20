
SYSCALL_DEFINE4(set_mempolicy_home_node, unsigned long, start, unsigned long, len,
		unsigned long, home_node, unsigned long, flags)
{
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma, *prev;
	struct mempolicy *new, *old;
	unsigned long end;
	int err = -ENOENT;
	VMA_ITERATOR(vmi, mm, start);

	start = untagged_addr(start);
	if (start & ~PAGE_MASK)
		return -EINVAL;
	/*
	 * flags is used for future extension if any.
	 */
	if (flags != 0)
		return -EINVAL;

	/*
	 * Check home_node is online to avoid accessing uninitialized
	 * NODE_DATA.
	 */
	if (home_node >= MAX_NUMNODES || !node_online(home_node))
		return -EINVAL;

	len = PAGE_ALIGN(len);
	end = start + len;

	if (end < start)
		return -EINVAL;
	if (end == start)
		return 0;
	mmap_write_lock(mm);
	prev = vma_prev(&vmi);
	for_each_vma_range(vmi, vma, end) {
		/*
		 * If any vma in the range got policy other than MPOL_BIND
		 * or MPOL_PREFERRED_MANY we return error. We don't reset
		 * the home node for vmas we already updated before.
		 */
		old = vma_policy(vma);
		if (!old)
			continue;
		if (old->mode != MPOL_BIND && old->mode != MPOL_PREFERRED_MANY) {
			err = -EOPNOTSUPP;
			break;
		}
		new = mpol_dup(old);
		if (IS_ERR(new)) {
			err = PTR_ERR(new);
			break;
		}

		new->home_node = home_node;
		err = mbind_range(&vmi, vma, &prev, start, end, new);
		mpol_put(new);
		if (err)
			break;
	}
	mmap_write_unlock(mm);
	return err;
}
