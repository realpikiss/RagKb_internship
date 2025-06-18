static inline void aio_poll_complete(struct aio_kiocb *iocb, __poll_t mask)
{
	struct file *file = iocb->poll.file;

	aio_complete(iocb, mangle_poll(mask), 0);
	fput(file);
}