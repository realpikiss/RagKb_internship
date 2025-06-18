static inline void __thread_fpu_end(struct task_struct *tsk)
{
	__thread_clear_has_fpu(tsk);
	if (!use_eager_fpu())
		stts();
}