static bool io_match_task(struct io_kiocb *head, struct task_struct *task,
			  bool cancel_all)
	__must_hold(&req->ctx->timeout_lock)
{
	if (task && head->task != task)
		return false;
	return cancel_all;
}
