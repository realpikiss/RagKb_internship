static int compat_copy_entries_to_user(unsigned int total_size,
				       struct xt_table *table,
				       void __user *userptr)
{
	struct xt_counters *counters;
	const struct xt_table_info *private = table->private;
	void __user *pos;
	unsigned int size;
	int ret = 0;
	unsigned int i = 0;
	struct arpt_entry *iter;

	counters = alloc_counters(table);
	if (IS_ERR(counters))
		return PTR_ERR(counters);

	pos = userptr;
	size = total_size;
	xt_entry_foreach(iter, private->entries, total_size) {
		ret = compat_copy_entry_to_user(iter, &pos,
						&size, counters, i++);
		if (ret != 0)
			break;
	}
	vfree(counters);
	return ret;
}