void remove_migration_pmd(struct page_vma_mapped_walk *pvmw, struct page *new)
{
	struct vm_area_struct *vma = pvmw->vma;
	struct mm_struct *mm = vma->vm_mm;
	unsigned long address = pvmw->address;
	unsigned long mmun_start = address & HPAGE_PMD_MASK;
	pmd_t pmde;
	swp_entry_t entry;

	if (!(pvmw->pmd && !pvmw->pte))
		return;

	entry = pmd_to_swp_entry(*pvmw->pmd);
	get_page(new);
	pmde = pmd_mkold(mk_huge_pmd(new, vma->vm_page_prot));
	if (pmd_swp_soft_dirty(*pvmw->pmd))
		pmde = pmd_mksoft_dirty(pmde);
	if (is_write_migration_entry(entry))
		pmde = maybe_pmd_mkwrite(pmde, vma);

	flush_cache_range(vma, mmun_start, mmun_start + HPAGE_PMD_SIZE);
	if (PageAnon(new))
		page_add_anon_rmap(new, vma, mmun_start, true);
	else
		page_add_file_rmap(new, true);
	set_pmd_at(mm, mmun_start, pvmw->pmd, pmde);
	if ((vma->vm_flags & VM_LOCKED) && !PageDoubleMap(new))
		mlock_vma_page(new);
	update_mmu_cache_pmd(vma, address, pvmw->pmd);
}
