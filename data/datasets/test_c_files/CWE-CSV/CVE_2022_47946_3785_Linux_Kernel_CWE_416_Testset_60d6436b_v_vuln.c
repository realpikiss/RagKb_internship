void __io_uring_task_cancel(void)
{
	struct io_uring_task *tctx = current->io_uring;
	DEFINE_WAIT(wait);
	s64 inflight;

	/* make sure overflow events are dropped */
	atomic_inc(&tctx->in_idle);

	/* trigger io_disable_sqo_submit() */
	if (tctx->sqpoll) {
		struct file *file;
		unsigned long index;

		xa_for_each(&tctx->xa, index, file)
			io_uring_cancel_sqpoll(file->private_data);
	}

	do {
		/* read completions before cancelations */
		inflight = tctx_inflight(tctx);
		if (!inflight)
			break;
		__io_uring_files_cancel(NULL);

		prepare_to_wait(&tctx->wait, &wait, TASK_UNINTERRUPTIBLE);

		/*
		 * If we've seen completions, retry without waiting. This
		 * avoids a race where a completion comes in before we did
		 * prepare_to_wait().
		 */
		if (inflight == tctx_inflight(tctx))
			schedule();
		finish_wait(&tctx->wait, &wait);
	} while (1);

	atomic_dec(&tctx->in_idle);

	io_uring_clean_tctx(tctx);
	/* all current's requests should be gone, we can kill tctx */
	__io_uring_free(current);
}
