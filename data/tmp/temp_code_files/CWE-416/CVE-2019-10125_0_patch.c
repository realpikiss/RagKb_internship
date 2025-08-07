static ssize_t aio_read(struct kiocb *req, const struct iocb *iocb,
			bool vectored, bool compat)
{
	struct iovec inline_vecs[UIO_FASTIOV], *iovec = inline_vecs;
	struct iov_iter iter;
	struct file *file;
	ssize_t ret;

	ret = aio_prep_rw(req, iocb);
	if (ret)
		return ret;
	file = req->ki_filp;
	if (unlikely(!(file->f_mode & FMODE_READ)))
		return -EBADF;
	ret = -EINVAL;
	if (unlikely(!file->f_op->read_iter))
		return -EINVAL;

	ret = aio_setup_rw(READ, iocb, &iovec, vectored, compat, &iter);
	if (ret)
		return ret;
	ret = rw_verify_area(READ, file, &req->ki_pos, iov_iter_count(&iter));
	if (!ret)
		aio_rw_done(req, call_read_iter(file, req, &iter));
	kfree(iovec);
	return ret;
}