static struct xt_counters *alloc_counters(const struct xt_table *table)
{
	unsigned int countersize;
	struct xt_counters *counters;
	const struct xt_table_info *private = table->private;

	/* We need atomic snapshot of counters: rest doesn't change
	 * (other than comefrom, which userspace doesn't care
	 * about).
	 */
	countersize = sizeof(struct xt_counters) * private->number;
	counters = vzalloc(countersize);

	if (counters == NULL)
		return ERR_PTR(-ENOMEM);

	get_counters(private, counters);

	return counters;
}
