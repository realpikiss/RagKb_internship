static void task_fd_install(
	struct binder_proc *proc, unsigned int fd, struct file *file)
{
	mutex_lock(&proc->files_lock);
	if (proc->files)
		__fd_install(proc->files, fd, file);
	mutex_unlock(&proc->files_lock);
}
