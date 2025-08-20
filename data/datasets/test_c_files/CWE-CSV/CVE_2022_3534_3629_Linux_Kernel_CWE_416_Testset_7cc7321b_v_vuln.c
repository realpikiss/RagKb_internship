void btf_dump__free(struct btf_dump *d)
{
	int i;

	if (IS_ERR_OR_NULL(d))
		return;

	free(d->type_states);
	if (d->cached_names) {
		/* any set cached name is owned by us and should be freed */
		for (i = 0; i <= d->last_id; i++) {
			if (d->cached_names[i])
				free((void *)d->cached_names[i]);
		}
	}
	free(d->cached_names);
	free(d->emit_queue);
	free(d->decl_stack);
	hashmap__free(d->type_names);
	hashmap__free(d->ident_names);

	free(d);
}
