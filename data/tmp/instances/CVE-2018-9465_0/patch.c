static int task_get_unused_fd_flags(struct binder_proc *proc, int flags)
{
	unsigned long rlim_cur;
	unsigned long irqs;
	int ret;

	mutex_lock(&proc->files_lock);
	if (proc->files == NULL) {
		ret = -ESRCH;
		goto err;
	}
	if (!lock_task_sighand(proc->tsk, &irqs)) {
		ret = -EMFILE;
		goto err;
	}
	rlim_cur = task_rlimit(proc->tsk, RLIMIT_NOFILE);
	unlock_task_sighand(proc->tsk, &irqs);

	ret = __alloc_fd(proc->files, 0, rlim_cur, flags);
err:
	mutex_unlock(&proc->files_lock);
	return ret;
}