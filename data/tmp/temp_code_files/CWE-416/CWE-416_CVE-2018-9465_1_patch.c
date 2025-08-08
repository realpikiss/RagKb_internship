static long task_close_fd(struct binder_proc *proc, unsigned int fd)
{
	int retval;

	mutex_lock(&proc->files_lock);
	if (proc->files == NULL) {
		retval = -ESRCH;
		goto err;
	}
	retval = __close_fd(proc->files, fd);
	/* can't restart close syscall because file table entry was cleared */
	if (unlikely(retval == -ERESTARTSYS ||
		     retval == -ERESTARTNOINTR ||
		     retval == -ERESTARTNOHAND ||
		     retval == -ERESTART_RESTARTBLOCK))
		retval = -EINTR;
err:
	mutex_unlock(&proc->files_lock);
	return retval;
}