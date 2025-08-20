int io_poll_add(struct io_kiocb *req, unsigned int issue_flags)
{
	struct io_poll *poll = io_kiocb_to_cmd(req);
	struct io_poll_table ipt;
	int ret;

	ipt.pt._qproc = io_poll_queue_proc;

	/*
	 * If sqpoll or single issuer, there is no contention for ->uring_lock
	 * and we'll end up holding it in tw handlers anyway.
	 */
	if (!(issue_flags & IO_URING_F_UNLOCKED) &&
	    (req->ctx->flags & (IORING_SETUP_SQPOLL | IORING_SETUP_SINGLE_ISSUER)))
		req->flags |= REQ_F_HASH_LOCKED;
	else
		req->flags &= ~REQ_F_HASH_LOCKED;

	ret = __io_arm_poll_handler(req, poll, &ipt, poll->events);
	if (ret) {
		io_req_set_res(req, ret, 0);
		return IOU_OK;
	}
	if (ipt.error) {
		req_set_fail(req);
		return ipt.error;
	}

	return IOU_ISSUE_SKIP_COMPLETE;
}
