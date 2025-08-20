static struct file *io_file_get_normal(struct io_kiocb *req, int fd)
{
	struct file *file = fget(fd);

	trace_io_uring_file_get(req->ctx, req, req->cqe.user_data, fd);

	/* we don't allow fixed io_uring files */
	if (file && file->f_op == &io_uring_fops)
		req->flags |= REQ_F_INFLIGHT;
	return file;
}
