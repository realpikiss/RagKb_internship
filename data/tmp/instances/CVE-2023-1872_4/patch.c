static struct iovec *__io_import_iovec(int rw, struct io_kiocb *req,
				       struct io_rw_state *s,
				       unsigned int issue_flags)
{
	struct iov_iter *iter = &s->iter;
	u8 opcode = req->opcode;
	struct iovec *iovec;
	void __user *buf;
	size_t sqe_len;
	ssize_t ret;

	if (opcode == IORING_OP_READ_FIXED || opcode == IORING_OP_WRITE_FIXED) {
		ret = io_import_fixed(req, rw, iter, issue_flags);
		if (ret)
			return ERR_PTR(ret);
		return NULL;
	}

	/* buffer index only valid with fixed read/write, or buffer select  */
	if (unlikely(req->buf_index && !(req->flags & REQ_F_BUFFER_SELECT)))
		return ERR_PTR(-EINVAL);

	buf = u64_to_user_ptr(req->rw.addr);
	sqe_len = req->rw.len;

	if (opcode == IORING_OP_READ || opcode == IORING_OP_WRITE) {
		if (req->flags & REQ_F_BUFFER_SELECT) {
			buf = io_rw_buffer_select(req, &sqe_len, issue_flags);
			if (IS_ERR(buf))
				return ERR_CAST(buf);
			req->rw.len = sqe_len;
		}

		ret = import_single_range(rw, buf, sqe_len, s->fast_iov, iter);
		if (ret)
			return ERR_PTR(ret);
		return NULL;
	}

	iovec = s->fast_iov;
	if (req->flags & REQ_F_BUFFER_SELECT) {
		ret = io_iov_buffer_select(req, iovec, issue_flags);
		if (ret)
			return ERR_PTR(ret);
		iov_iter_init(iter, rw, iovec, 1, iovec->iov_len);
		return NULL;
	}

	ret = __import_iovec(rw, buf, sqe_len, UIO_FASTIOV, &iovec, iter,
			      req->ctx->compat);
	if (unlikely(ret < 0))
		return ERR_PTR(ret);
	return iovec;
}