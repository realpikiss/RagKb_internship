static void smaps_pte_entry(pte_t *pte, unsigned long addr,
		struct mm_walk *walk)
{
	struct mem_size_stats *mss = walk->private;
	struct vm_area_struct *vma = walk->vma;
	bool locked = !!(vma->vm_flags & VM_LOCKED);
	struct page *page = NULL;
	bool migration = false;

	if (pte_present(*pte)) {
		page = vm_normal_page(vma, addr, *pte);
	} else if (is_swap_pte(*pte)) {
		swp_entry_t swpent = pte_to_swp_entry(*pte);

		if (!non_swap_entry(swpent)) {
			int mapcount;

			mss->swap += PAGE_SIZE;
			mapcount = swp_swapcount(swpent);
			if (mapcount >= 2) {
				u64 pss_delta = (u64)PAGE_SIZE << PSS_SHIFT;

				do_div(pss_delta, mapcount);
				mss->swap_pss += pss_delta;
			} else {
				mss->swap_pss += (u64)PAGE_SIZE << PSS_SHIFT;
			}
		} else if (is_pfn_swap_entry(swpent)) {
			if (is_migration_entry(swpent))
				migration = true;
			page = pfn_swap_entry_to_page(swpent);
		}
	} else {
		smaps_pte_hole_lookup(addr, walk);
		return;
	}

	if (!page)
		return;

	smaps_account(mss, page, false, pte_young(*pte), pte_dirty(*pte),
		      locked, migration);
}
