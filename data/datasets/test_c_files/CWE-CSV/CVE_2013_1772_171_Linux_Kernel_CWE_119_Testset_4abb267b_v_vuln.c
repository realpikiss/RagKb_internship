void console_unlock(void)
{
	unsigned long flags;
	unsigned _con_start, _log_end;
	unsigned wake_klogd = 0, retry = 0;

	if (console_suspended) {
		up(&console_sem);
		return;
	}

	console_may_schedule = 0;

again:
	for ( ; ; ) {
		raw_spin_lock_irqsave(&logbuf_lock, flags);
		wake_klogd |= log_start - log_end;
		if (con_start == log_end)
			break;			/* Nothing to print */
		_con_start = con_start;
		_log_end = log_end;
		con_start = log_end;		/* Flush */
		raw_spin_unlock(&logbuf_lock);
		stop_critical_timings();	/* don't trace print latency */
		call_console_drivers(_con_start, _log_end);
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
	if (con_start != log_end)
		retry = 1;
	raw_spin_unlock_irqrestore(&logbuf_lock, flags);

	if (retry && console_trylock())
		goto again;

	if (wake_klogd)
		wake_up_klogd();
}
