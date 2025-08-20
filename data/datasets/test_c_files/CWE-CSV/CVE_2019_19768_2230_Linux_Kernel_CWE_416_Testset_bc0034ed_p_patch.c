void blk_add_driver_data(struct request_queue *q,
			 struct request *rq,
			 void *data, size_t len)
{
	struct blk_trace *bt;

	rcu_read_lock();
	bt = rcu_dereference(q->blk_trace);
	if (likely(!bt)) {
		rcu_read_unlock();
		return;
	}

	__blk_add_trace(bt, blk_rq_trace_sector(rq), blk_rq_bytes(rq), 0, 0,
				BLK_TA_DRV_DATA, 0, len, data,
				blk_trace_request_get_cgid(q, rq));
	rcu_read_unlock();
}
