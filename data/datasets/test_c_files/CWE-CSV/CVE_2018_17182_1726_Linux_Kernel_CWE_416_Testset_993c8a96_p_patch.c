static inline void vmacache_invalidate(struct mm_struct *mm)
{
	mm->vmacache_seqnum++;
}
