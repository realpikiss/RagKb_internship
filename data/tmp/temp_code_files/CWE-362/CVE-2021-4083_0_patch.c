static struct file *__fget_files(struct files_struct *files, unsigned int fd,
				 fmode_t mask, unsigned int refs)
{
	struct file *file;

	rcu_read_lock();
loop:
	file = files_lookup_fd_rcu(files, fd);
	if (file) {
		/* File object ref couldn't be taken.
		 * dup2() atomicity guarantee is the reason
		 * we loop to catch the new file (or NULL pointer)
		 */
		if (file->f_mode & mask)
			file = NULL;
		else if (!get_file_rcu_many(file, refs))
			goto loop;
		else if (files_lookup_fd_raw(files, fd) != file) {
			fput_many(file, refs);
			goto loop;
		}
	}
	rcu_read_unlock();

	return file;
}