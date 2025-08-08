static inline void aio_poll_complete(struct aio_kiocb *iocb, __poll_t mask)
{
	aio_complete(iocb, mangle_poll(mask), 0);
}