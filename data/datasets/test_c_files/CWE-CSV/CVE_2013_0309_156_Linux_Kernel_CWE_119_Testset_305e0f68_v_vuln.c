static inline int pmd_large(pmd_t pte)
{
	return (pmd_flags(pte) & (_PAGE_PSE | _PAGE_PRESENT)) ==
		(_PAGE_PSE | _PAGE_PRESENT);
}
