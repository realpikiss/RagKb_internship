static void print_binder_transaction_ilocked(struct seq_file *m,
					     struct binder_proc *proc,
					     const char *prefix,
					     struct binder_transaction *t)
{
	struct binder_proc *to_proc;
	struct binder_buffer *buffer = t->buffer;

	spin_lock(&t->lock);
	to_proc = t->to_proc;
	seq_printf(m,
		   "%s %d: %pK from %d:%d to %d:%d code %x flags %x pri %ld r%d",
		   prefix, t->debug_id, t,
		   t->from ? t->from->proc->pid : 0,
		   t->from ? t->from->pid : 0,
		   to_proc ? to_proc->pid : 0,
		   t->to_thread ? t->to_thread->pid : 0,
		   t->code, t->flags, t->priority, t->need_reply);
	spin_unlock(&t->lock);

	if (proc != to_proc) {
		/*
		 * Can only safely deref buffer if we are holding the
		 * correct proc inner lock for this node
		 */
		seq_puts(m, "\n");
		return;
	}

	if (buffer == NULL) {
		seq_puts(m, " buffer free\n");
		return;
	}
	if (buffer->target_node)
		seq_printf(m, " node %d", buffer->target_node->debug_id);
	seq_printf(m, " size %zd:%zd data %pK\n",
		   buffer->data_size, buffer->offsets_size,
		   buffer->data);
}