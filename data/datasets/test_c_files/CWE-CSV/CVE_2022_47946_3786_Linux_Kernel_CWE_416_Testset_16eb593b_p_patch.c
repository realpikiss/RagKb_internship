static void __io_req_task_submit(struct io_kiocb *req)
{
	struct io_ring_ctx *ctx = req->ctx;

	/* ctx stays valid until unlock, even if we drop all ours ctx->refs */
	mutex_lock(&ctx->uring_lock);
	if (!(current->flags & PF_EXITING) && !current->in_execve)
		__io_queue_sqe(req);
	else
		__io_req_task_cancel(req, -EFAULT);
	mutex_unlock(&ctx->uring_lock);
}
