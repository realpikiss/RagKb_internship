static bool io_match_task_safe(struct io_kiocb *head, struct task_struct *task,
			       bool cancel_all)
{
	bool matched;

	if (task && head->task != task)
		return false;
	if (cancel_all)
		return true;

	if (head->flags & REQ_F_LINK_TIMEOUT) {
		struct io_ring_ctx *ctx = head->ctx;

		/* protect against races with linked timeouts */
		spin_lock_irq(&ctx->timeout_lock);
		matched = io_match_linked(head);
		spin_unlock_irq(&ctx->timeout_lock);
	} else {
		matched = io_match_linked(head);
	}
	return matched;
}
