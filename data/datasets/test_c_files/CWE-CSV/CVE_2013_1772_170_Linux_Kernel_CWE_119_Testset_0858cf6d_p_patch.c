static ssize_t kmsg_writev(struct kiocb *iocb, const struct iovec *iv,
			   unsigned long count, loff_t pos)
{
	char *buf, *line;
	int i;
	int level = default_message_loglevel;
	int facility = 1;	/* LOG_USER */
	size_t len = iov_length(iv, count);
	ssize_t ret = len;

	if (len > 1024)
		return -EINVAL;
	buf = kmalloc(len+1, GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;

	line = buf;
	for (i = 0; i < count; i++) {
		if (copy_from_user(line, iv[i].iov_base, iv[i].iov_len))
			goto out;
		line += iv[i].iov_len;
	}

	/*
	 * Extract and skip the syslog prefix <[0-9]*>. Coming from userspace
	 * the decimal value represents 32bit, the lower 3 bit are the log
	 * level, the rest are the log facility.
	 *
	 * If no prefix or no userspace facility is specified, we
	 * enforce LOG_USER, to be able to reliably distinguish
	 * kernel-generated messages from userspace-injected ones.
	 */
	line = buf;
	if (line[0] == '<') {
		char *endp = NULL;

		i = simple_strtoul(line+1, &endp, 10);
		if (endp && endp[0] == '>') {
			level = i & 7;
			if (i >> 3)
				facility = i >> 3;
			endp++;
			len -= endp - line;
			line = endp;
		}
	}
	line[len] = '\0';

	printk_emit(facility, level, NULL, 0, "%s", line);
out:
	kfree(buf);
	return ret;
}
