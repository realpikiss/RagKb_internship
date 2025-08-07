static inline void drop_init_fpu(struct task_struct *tsk)
{
	if (!use_eager_fpu())
		drop_fpu(tsk);
	else {
		if (use_xsave())
			xrstor_state(init_xstate_buf, -1);
		else
			fxrstor_checking(&init_xstate_buf->i387);
	}
}