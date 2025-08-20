static int __io_arm_poll_handler(struct io_kiocb *req,
				 struct io_poll *poll,
				 struct io_poll_table *ipt, __poll_t mask)
{
	struct io_ring_ctx *ctx = req->ctx;
	int v;

	INIT_HLIST_NODE(&req->hash_node);
	req->work.cancel_seq = atomic_read(&ctx->cancel_seq);
	io_init_poll_iocb(poll, mask, io_poll_wake);
	poll->file = req->file;

	req->apoll_events = poll->events;

	ipt->pt._key = mask;
	ipt->req = req;
	ipt->error = 0;
	ipt->nr_entries = 0;

	/*
	 * Take the ownership to delay any tw execution up until we're done
	 * with poll arming. see io_poll_get_ownership().
	 */
	atomic_set(&req->poll_refs, 1);
	mask = vfs_poll(req->file, &ipt->pt) & poll->events;

	if (mask &&
	   ((poll->events & (EPOLLET|EPOLLONESHOT)) == (EPOLLET|EPOLLONESHOT))) {
		io_poll_remove_entries(req);
		/* no one else has access to the req, forget about the ref */
		return mask;
	}

	if (!mask && unlikely(ipt->error || !ipt->nr_entries)) {
		io_poll_remove_entries(req);
		if (!ipt->error)
			ipt->error = -EINVAL;
		return 0;
	}

	io_poll_req_insert(req);

	if (mask && (poll->events & EPOLLET)) {
		/* can't multishot if failed, just queue the event we've got */
		if (unlikely(ipt->error || !ipt->nr_entries)) {
			poll->events |= EPOLLONESHOT;
			req->apoll_events |= EPOLLONESHOT;
			ipt->error = 0;
		}
		__io_poll_execute(req, mask, poll->events);
		return 0;
	}

	/*
	 * Release ownership. If someone tried to queue a tw while it was
	 * locked, kick it off for them.
	 */
	v = atomic_dec_return(&req->poll_refs);
	if (unlikely(v & IO_POLL_REF_MASK))
		__io_poll_execute(req, 0, poll->events);
	return 0;
}
