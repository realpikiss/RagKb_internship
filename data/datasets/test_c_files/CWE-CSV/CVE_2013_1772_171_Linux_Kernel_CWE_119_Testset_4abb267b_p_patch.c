void console_unlock(void)
{
	static u64 seen_seq;
	unsigned long flags;
	bool wake_klogd = false;
	bool retry;

	if (console_suspended) {
		up(&console_sem);
		return;
	}

	console_may_schedule = 0;

again:
	for (;;) {
		struct log *msg;
		static char text[LOG_LINE_MAX];
		size_t len;
		int level;

		raw_spin_lock_irqsave(&logbuf_lock, flags);
		if (seen_seq != log_next_seq) {
			wake_klogd = true;
			seen_seq = log_next_seq;
		}

		if (console_seq < log_first_seq) {
			/* messages are gone, move to first one */
			console_seq = log_first_seq;
			console_idx = log_first_idx;
		}

		if (console_seq == log_next_seq)
			break;

		msg = log_from_idx(console_idx);
		level = msg->level & 7;
		len = msg->text_len;
		if (len+1 >= sizeof(text))
			len = sizeof(text)-1;
		memcpy(text, log_text(msg), len);
		text[len++] = '\n';

		console_idx = log_next(console_idx);
		console_seq++;
		raw_spin_unlock(&logbuf_lock);

		stop_critical_timings();	/* don't trace print latency */
		call_console_drivers(level, text, len);
		start_critical_timings();
		local_irq_restore(flags);
	}
	console_locked = 0;

	/* Release the exclusive_console once it is used */
	if (unlikely(exclusive_console))
		exclusive_console = NULL;

	raw_spin_unlock(&logbuf_lock);

	up(&console_sem);

	/*
	 * Someone could have filled up the buffer again, so re-check if there's
	 * something to flush. In case we cannot trylock the console_sem again,
	 * there's a new owner and the console_unlock() from them will do the
	 * flush, no worries.
	 */
	raw_spin_lock(&logbuf_lock);
	retry = console_seq != log_next_seq;
	raw_spin_unlock_irqrestore(&logbuf_lock, flags);

	if (retry && console_trylock())
		goto again;

	if (wake_klogd)
		wake_up_klogd();
}
