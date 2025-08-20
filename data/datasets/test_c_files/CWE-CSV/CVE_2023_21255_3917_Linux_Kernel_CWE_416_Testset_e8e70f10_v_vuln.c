static int binder_proc_transaction(struct binder_transaction *t,
				    struct binder_proc *proc,
				    struct binder_thread *thread)
{
	struct binder_node *node = t->buffer->target_node;
	bool oneway = !!(t->flags & TF_ONE_WAY);
	bool pending_async = false;
	struct binder_transaction *t_outdated = NULL;
	bool frozen = false;

	BUG_ON(!node);
	binder_node_lock(node);
	if (oneway) {
		BUG_ON(thread);
		if (node->has_async_transaction)
			pending_async = true;
		else
			node->has_async_transaction = true;
	}

	binder_inner_proc_lock(proc);
	if (proc->is_frozen) {
		frozen = true;
		proc->sync_recv |= !oneway;
		proc->async_recv |= oneway;
	}

	if ((frozen && !oneway) || proc->is_dead ||
			(thread && thread->is_dead)) {
		binder_inner_proc_unlock(proc);
		binder_node_unlock(node);
		return frozen ? BR_FROZEN_REPLY : BR_DEAD_REPLY;
	}

	if (!thread && !pending_async)
		thread = binder_select_thread_ilocked(proc);

	if (thread) {
		binder_enqueue_thread_work_ilocked(thread, &t->work);
	} else if (!pending_async) {
		binder_enqueue_work_ilocked(&t->work, &proc->todo);
	} else {
		if ((t->flags & TF_UPDATE_TXN) && frozen) {
			t_outdated = binder_find_outdated_transaction_ilocked(t,
									      &node->async_todo);
			if (t_outdated) {
				binder_debug(BINDER_DEBUG_TRANSACTION,
					     "txn %d supersedes %d\n",
					     t->debug_id, t_outdated->debug_id);
				list_del_init(&t_outdated->work.entry);
				proc->outstanding_txns--;
			}
		}
		binder_enqueue_work_ilocked(&t->work, &node->async_todo);
	}

	if (!pending_async)
		binder_wakeup_thread_ilocked(proc, thread, !oneway /* sync */);

	proc->outstanding_txns++;
	binder_inner_proc_unlock(proc);
	binder_node_unlock(node);

	/*
	 * To reduce potential contention, free the outdated transaction and
	 * buffer after releasing the locks.
	 */
	if (t_outdated) {
		struct binder_buffer *buffer = t_outdated->buffer;

		t_outdated->buffer = NULL;
		buffer->transaction = NULL;
		trace_binder_transaction_update_buffer_release(buffer);
		binder_transaction_buffer_release(proc, NULL, buffer, 0, 0);
		binder_alloc_free_buf(&proc->alloc, buffer);
		kfree(t_outdated);
		binder_stats_deleted(BINDER_STAT_TRANSACTION);
	}

	if (oneway && frozen)
		return BR_TRANSACTION_PENDING_FROZEN;

	return 0;
}
