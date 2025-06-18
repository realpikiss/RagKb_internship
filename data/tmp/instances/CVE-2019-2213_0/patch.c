static void binder_free_transaction(struct binder_transaction *t)
{
	struct binder_proc *target_proc = t->to_proc;

	if (target_proc) {
		binder_inner_proc_lock(target_proc);
		if (t->buffer)
			t->buffer->transaction = NULL;
		binder_inner_proc_unlock(target_proc);
	}
	/*
	 * If the transaction has no target_proc, then
	 * t->buffer->transaction has already been cleared.
	 */
	binder_free_txn_fixups(t);
	kfree(t);
	binder_stats_deleted(BINDER_STAT_TRANSACTION);
}