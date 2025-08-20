static inline void io_req_track_inflight(struct io_kiocb *req)
{
	if (!(req->flags & REQ_F_INFLIGHT)) {
		req->flags |= REQ_F_INFLIGHT;
		atomic_inc(&req->task->io_uring->inflight_tracked);
	}
}
