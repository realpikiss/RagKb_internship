static inline int pmd_large(pmd_t pte)
{
	return pmd_flags(pte) & _PAGE_PSE;
}
