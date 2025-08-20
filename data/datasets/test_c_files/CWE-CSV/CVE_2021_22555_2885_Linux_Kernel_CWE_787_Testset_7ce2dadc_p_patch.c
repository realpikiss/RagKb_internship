void xt_compat_target_from_user(struct xt_entry_target *t, void **dstptr,
				unsigned int *size)
{
	const struct xt_target *target = t->u.kernel.target;
	struct compat_xt_entry_target *ct = (struct compat_xt_entry_target *)t;
	int off = xt_compat_target_offset(target);
	u_int16_t tsize = ct->u.user.target_size;
	char name[sizeof(t->u.user.name)];

	t = *dstptr;
	memcpy(t, ct, sizeof(*ct));
	if (target->compat_from_user)
		target->compat_from_user(t->data, ct->data);
	else
		memcpy(t->data, ct->data, tsize - sizeof(*ct));

	tsize += off;
	t->u.user.target_size = tsize;
	strlcpy(name, target->name, sizeof(name));
	module_put(target->me);
	strncpy(t->u.user.name, name, sizeof(t->u.user.name));

	*size += off;
	*dstptr += tsize;
}
