static ssize_t kmsg_writev(struct kiocb *iocb, const struct iovec *iv,
			   unsigned long count, loff_t pos)
{
	char *line, *p;
	int i;
	ssize_t ret = -EFAULT;
	size_t len = iov_length(iv, count);

	line = kmalloc(len + 1, GFP_KERNEL);
	if (line == NULL)
		return -ENOMEM;

	/*
	 * copy all vectors into a single string, to ensure we do
	 * not interleave our log line with other printk calls
	 */
	p = line;
	for (i = 0; i < count; i++) {
		if (copy_from_user(p, iv[i].iov_base, iv[i].iov_len))
			goto out;
		p += iv[i].iov_len;
	}
	p[0] = '\0';

	ret = printk("%s", line);
	/* printk can add a prefix */
	if (ret > len)
		ret = len;
out:
	kfree(line);
	return ret;
}
