void __init setup_log_buf(int early)
{
	unsigned long flags;
	unsigned start, dest_idx, offset;
	char *new_log_buf;
	int free;

	if (!new_log_buf_len)
		return;

	if (early) {
		unsigned long mem;

		mem = memblock_alloc(new_log_buf_len, PAGE_SIZE);
		if (!mem)
			return;
		new_log_buf = __va(mem);
	} else {
		new_log_buf = alloc_bootmem_nopanic(new_log_buf_len);
	}

	if (unlikely(!new_log_buf)) {
		pr_err("log_buf_len: %ld bytes not available\n",
			new_log_buf_len);
		return;
	}

	raw_spin_lock_irqsave(&logbuf_lock, flags);
	log_buf_len = new_log_buf_len;
	log_buf = new_log_buf;
	new_log_buf_len = 0;
	free = __LOG_BUF_LEN - log_end;

	offset = start = min(con_start, log_start);
	dest_idx = 0;
	while (start != log_end) {
		unsigned log_idx_mask = start & (__LOG_BUF_LEN - 1);

		log_buf[dest_idx] = __log_buf[log_idx_mask];
		start++;
		dest_idx++;
	}
	log_start -= offset;
	con_start -= offset;
	log_end -= offset;
	raw_spin_unlock_irqrestore(&logbuf_lock, flags);

	pr_info("log_buf_len: %d\n", log_buf_len);
	pr_info("early log buf free: %d(%d%%)\n",
		free, (free * 100) / __LOG_BUF_LEN);
}
