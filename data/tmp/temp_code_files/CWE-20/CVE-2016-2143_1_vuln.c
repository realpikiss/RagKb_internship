static inline void arch_dup_mmap(struct mm_struct *oldmm,
				 struct mm_struct *mm)
{
	if (oldmm->context.asce_limit < mm->context.asce_limit)
		crst_table_downgrade(mm, oldmm->context.asce_limit);
}