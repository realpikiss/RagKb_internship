static inline void save_init_fpu(struct task_struct *tsk)
{
	WARN_ON_ONCE(!__thread_has_fpu(tsk));

	if (use_eager_fpu()) {
		__save_fpu(tsk);
		return;
	}

	preempt_disable();
	__save_init_fpu(tsk);
	__thread_fpu_end(tsk);
	preempt_enable();
}