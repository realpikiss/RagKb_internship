static inline void iocb_put(struct aio_kiocb *iocb)
{
	if (refcount_read(&iocb->ki_refcnt) == 0 ||
	    refcount_dec_and_test(&iocb->ki_refcnt)) {
		if (iocb->ki_filp)
			fput(iocb->ki_filp);
		percpu_ref_put(&iocb->ki_ctx->reqs);
		kmem_cache_free(kiocb_cachep, iocb);
	}
}