struct page *follow_devmap_pmd(struct vm_area_struct *vma, unsigned long addr,
		pmd_t *pmd, int flags, struct dev_pagemap **pgmap)
{
	unsigned long pfn = pmd_pfn(*pmd);
	struct mm_struct *mm = vma->vm_mm;
	struct page *page;

	assert_spin_locked(pmd_lockptr(mm, pmd));

	/*
	 * When we COW a devmap PMD entry, we split it into PTEs, so we should
	 * not be in this function with `flags & FOLL_COW` set.
	 */
	WARN_ONCE(flags & FOLL_COW, "mm: In follow_devmap_pmd with FOLL_COW set");

	/* FOLL_GET and FOLL_PIN are mutually exclusive. */
	if (WARN_ON_ONCE((flags & (FOLL_PIN | FOLL_GET)) ==
			 (FOLL_PIN | FOLL_GET)))
		return NULL;

	if (flags & FOLL_WRITE && !pmd_write(*pmd))
		return NULL;

	if (pmd_present(*pmd) && pmd_devmap(*pmd))
		/* pass */;
	else
		return NULL;

	if (flags & FOLL_TOUCH)
		touch_pmd(vma, addr, pmd, flags & FOLL_WRITE);

	/*
	 * device mapped pages can only be returned if the
	 * caller will manage the page reference count.
	 */
	if (!(flags & (FOLL_GET | FOLL_PIN)))
		return ERR_PTR(-EEXIST);

	pfn += (addr & ~PMD_MASK) >> PAGE_SHIFT;
	*pgmap = get_dev_pagemap(pfn, *pgmap);
	if (!*pgmap)
		return ERR_PTR(-EFAULT);
	page = pfn_to_page(pfn);
	if (!try_grab_page(page, flags))
		page = ERR_PTR(-ENOMEM);

	return page;
}
