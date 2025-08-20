static void print_binder_ref(struct seq_file *m, struct binder_ref *ref)
{
	seq_printf(m, "  ref %d: desc %d %snode %d s %d w %d d %pK\n",
		   ref->data.debug_id, ref->data.desc,
		   ref->node->proc ? "" : "dead ",
		   ref->node->debug_id, ref->data.strong,
		   ref->data.weak, ref->death);
}
