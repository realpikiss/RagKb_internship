static inline bool kdb_check_flags(kdb_cmdflags_t flags, int permissions,
				   bool no_args)
{
	/* permissions comes from userspace so needs massaging slightly */
	permissions &= KDB_ENABLE_MASK;
	permissions |= KDB_ENABLE_ALWAYS_SAFE;

	/* some commands change group when launched with no arguments */
	if (no_args)
		permissions |= permissions << KDB_ENABLE_NO_ARGS_SHIFT;

	flags |= KDB_ENABLE_ALL;

	return permissions & flags;
}
