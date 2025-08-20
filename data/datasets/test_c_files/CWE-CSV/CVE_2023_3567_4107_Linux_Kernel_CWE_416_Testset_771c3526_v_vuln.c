static ssize_t
vcs_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	struct inode *inode = file_inode(file);
	struct vc_data *vc;
	struct vcs_poll_data *poll;
	unsigned int read;
	ssize_t ret;
	char *con_buf;
	loff_t pos;
	bool viewed, attr, uni_mode;

	con_buf = (char *) __get_free_page(GFP_KERNEL);
	if (!con_buf)
		return -ENOMEM;

	pos = *ppos;

	/* Select the proper current console and verify
	 * sanity of the situation under the console lock.
	 */
	console_lock();

	uni_mode = use_unicode(inode);
	attr = use_attributes(inode);
	ret = -ENXIO;
	vc = vcs_vc(inode, &viewed);
	if (!vc)
		goto unlock_out;

	ret = -EINVAL;
	if (pos < 0)
		goto unlock_out;
	/* we enforce 32-bit alignment for pos and count in unicode mode */
	if (uni_mode && (pos | count) & 3)
		goto unlock_out;

	poll = file->private_data;
	if (count && poll)
		poll->event = 0;
	read = 0;
	ret = 0;
	while (count) {
		unsigned int this_round, skip = 0;
		int size;

		/* Check whether we are above size each round,
		 * as copy_to_user at the end of this loop
		 * could sleep.
		 */
		size = vcs_size(vc, attr, uni_mode);
		if (size < 0) {
			if (read)
				break;
			ret = size;
			goto unlock_out;
		}
		if (pos >= size)
			break;
		if (count > size - pos)
			count = size - pos;

		this_round = count;
		if (this_round > CON_BUF_SIZE)
			this_round = CON_BUF_SIZE;

		/* Perform the whole read into the local con_buf.
		 * Then we can drop the console spinlock and safely
		 * attempt to move it to userspace.
		 */

		if (uni_mode) {
			ret = vcs_read_buf_uni(vc, con_buf, pos, this_round,
					viewed);
			if (ret)
				break;
		} else if (!attr) {
			vcs_read_buf_noattr(vc, con_buf, pos, this_round,
					viewed);
		} else {
			this_round = vcs_read_buf(vc, con_buf, pos, this_round,
					viewed, &skip);
		}

		/* Finally, release the console semaphore while we push
		 * all the data to userspace from our temporary buffer.
		 *
		 * AKPM: Even though it's a semaphore, we should drop it because
		 * the pagefault handling code may want to call printk().
		 */

		console_unlock();
		ret = copy_to_user(buf, con_buf + skip, this_round);
		console_lock();

		if (ret) {
			read += this_round - ret;
			ret = -EFAULT;
			break;
		}
		buf += this_round;
		pos += this_round;
		read += this_round;
		count -= this_round;
	}
	*ppos += read;
	if (read)
		ret = read;
unlock_out:
	console_unlock();
	free_page((unsigned long) con_buf);
	return ret;
}
