void xt_compat_match_from_user(struct xt_entry_match *m, void **dstptr,
			       unsigned int *size)
{
	const struct xt_match *match = m->u.kernel.match;
	struct compat_xt_entry_match *cm = (struct compat_xt_entry_match *)m;
	int off = xt_compat_match_offset(match);
	u_int16_t msize = cm->u.user.match_size;
	char name[sizeof(m->u.user.name)];

	m = *dstptr;
	memcpy(m, cm, sizeof(*cm));
	if (match->compat_from_user)
		match->compat_from_user(m->data, cm->data);
	else
		memcpy(m->data, cm->data, msize - sizeof(*cm));

	msize += off;
	m->u.user.match_size = msize;
	strlcpy(name, match->name, sizeof(name));
	module_put(match->me);
	strncpy(m->u.user.name, name, sizeof(m->u.user.name));

	*size += off;
	*dstptr += msize;
}
