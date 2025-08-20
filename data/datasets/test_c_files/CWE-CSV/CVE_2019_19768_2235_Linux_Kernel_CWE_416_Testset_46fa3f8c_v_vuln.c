static void blk_add_trace_unplug(void *ignore, struct request_queue *q,
				    unsigned int depth, bool explicit)
{
	struct blk_trace *bt = q->blk_trace;

	if (bt) {
		__be64 rpdu = cpu_to_be64(depth);
		u32 what;

		if (explicit)
			what = BLK_TA_UNPLUG_IO;
		else
			what = BLK_TA_UNPLUG_TIMER;

		__blk_add_trace(bt, 0, 0, 0, 0, what, 0, sizeof(rpdu), &rpdu, 0);
	}
}
