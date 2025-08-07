static inline void fpu_copy(struct task_struct *dst, struct task_struct *src)
{
	if (use_eager_fpu()) {
		memset(&dst->thread.fpu.state->xsave, 0, xstate_size);
		__save_fpu(dst);
	} else {
		struct fpu *dfpu = &dst->thread.fpu;
		struct fpu *sfpu = &src->thread.fpu;

		unlazy_fpu(src);
		memcpy(dfpu->state, sfpu->state, xstate_size);
	}
}