static inline int pmd_present(pmd_t pmd)
{
	return pmd_flags(pmd) & _PAGE_PRESENT;
}