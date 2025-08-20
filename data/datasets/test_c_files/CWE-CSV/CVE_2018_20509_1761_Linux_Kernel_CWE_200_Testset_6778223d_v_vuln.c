static struct binder_ref *binder_get_ref(struct binder_proc *proc,
					 u32 desc, bool need_strong_ref)
{
	struct rb_node *n = proc->refs_by_desc.rb_node;
	struct binder_ref *ref;

	while (n) {
		ref = rb_entry(n, struct binder_ref, rb_node_desc);

		if (desc < ref->desc) {
			n = n->rb_left;
		} else if (desc > ref->desc) {
			n = n->rb_right;
		} else if (need_strong_ref && !ref->strong) {
			binder_user_error("tried to use weak ref as strong ref\n");
			return NULL;
		} else {
			return ref;
		}
	}
	return NULL;
}
