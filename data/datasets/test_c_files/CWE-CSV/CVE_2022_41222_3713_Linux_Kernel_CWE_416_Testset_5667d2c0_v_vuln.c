unsigned long move_page_tables(struct vm_area_struct *vma,
		unsigned long old_addr, struct vm_area_struct *new_vma,
		unsigned long new_addr, unsigned long len,
		bool need_rmap_locks)
{
	unsigned long extent, old_end;
	struct mmu_notifier_range range;
	pmd_t *old_pmd, *new_pmd;
	pud_t *old_pud, *new_pud;

	old_end = old_addr + len;
	flush_cache_range(vma, old_addr, old_end);

	mmu_notifier_range_init(&range, MMU_NOTIFY_UNMAP, 0, vma, vma->vm_mm,
				old_addr, old_end);
	mmu_notifier_invalidate_range_start(&range);

	for (; old_addr < old_end; old_addr += extent, new_addr += extent) {
		cond_resched();
		/*
		 * If extent is PUD-sized try to speed up the move by moving at the
		 * PUD level if possible.
		 */
		extent = get_extent(NORMAL_PUD, old_addr, old_end, new_addr);

		old_pud = get_old_pud(vma->vm_mm, old_addr);
		if (!old_pud)
			continue;
		new_pud = alloc_new_pud(vma->vm_mm, vma, new_addr);
		if (!new_pud)
			break;
		if (pud_trans_huge(*old_pud) || pud_devmap(*old_pud)) {
			if (extent == HPAGE_PUD_SIZE) {
				move_pgt_entry(HPAGE_PUD, vma, old_addr, new_addr,
					       old_pud, new_pud, need_rmap_locks);
				/* We ignore and continue on error? */
				continue;
			}
		} else if (IS_ENABLED(CONFIG_HAVE_MOVE_PUD) && extent == PUD_SIZE) {

			if (move_pgt_entry(NORMAL_PUD, vma, old_addr, new_addr,
					   old_pud, new_pud, need_rmap_locks))
				continue;
		}

		extent = get_extent(NORMAL_PMD, old_addr, old_end, new_addr);
		old_pmd = get_old_pmd(vma->vm_mm, old_addr);
		if (!old_pmd)
			continue;
		new_pmd = alloc_new_pmd(vma->vm_mm, vma, new_addr);
		if (!new_pmd)
			break;
		if (is_swap_pmd(*old_pmd) || pmd_trans_huge(*old_pmd) ||
		    pmd_devmap(*old_pmd)) {
			if (extent == HPAGE_PMD_SIZE &&
			    move_pgt_entry(HPAGE_PMD, vma, old_addr, new_addr,
					   old_pmd, new_pmd, need_rmap_locks))
				continue;
			split_huge_pmd(vma, old_pmd, old_addr);
			if (pmd_trans_unstable(old_pmd))
				continue;
		} else if (IS_ENABLED(CONFIG_HAVE_MOVE_PMD) &&
			   extent == PMD_SIZE) {
			/*
			 * If the extent is PMD-sized, try to speed the move by
			 * moving at the PMD level if possible.
			 */
			if (move_pgt_entry(NORMAL_PMD, vma, old_addr, new_addr,
					   old_pmd, new_pmd, need_rmap_locks))
				continue;
		}

		if (pte_alloc(new_vma->vm_mm, new_pmd))
			break;
		move_ptes(vma, old_pmd, old_addr, old_addr + extent, new_vma,
			  new_pmd, new_addr, need_rmap_locks);
	}

	mmu_notifier_invalidate_range_end(&range);

	return len + old_addr - old_end;	/* how much done */
}
