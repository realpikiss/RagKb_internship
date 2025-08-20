static void blk_add_trace_split(void *ignore,
				struct request_queue *q, struct bio *bio,
				unsigned int pdu)
{
	struct blk_trace *bt;

	rcu_read_lock();
	bt = rcu_dereference(q->blk_trace);
	if (bt) {
		__be64 rpdu = cpu_to_be64(pdu);

		__blk_add_trace(bt, bio->bi_iter.bi_sector,
				bio->bi_iter.bi_size, bio_op(bio), bio->bi_opf,
				BLK_TA_SPLIT, bio->bi_status, sizeof(rpdu),
				&rpdu, blk_trace_bio_get_cgid(q, bio));
	}
	rcu_read_unlock();
}
