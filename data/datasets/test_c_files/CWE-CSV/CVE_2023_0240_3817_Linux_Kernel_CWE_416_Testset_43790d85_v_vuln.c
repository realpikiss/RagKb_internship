static inline void io_req_init_async(struct io_kiocb *req)
{
	if (req->flags & REQ_F_WORK_INITIALIZED)
		return;

	memset(&req->work, 0, sizeof(req->work));
	req->flags |= REQ_F_WORK_INITIALIZED;
	req->work.identity = &req->identity;
}
