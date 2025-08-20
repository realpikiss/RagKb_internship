static void k_fn(struct vc_data *vc, unsigned char value, char up_flag)
{
	if (up_flag)
		return;

	if ((unsigned)value < ARRAY_SIZE(func_table)) {
		unsigned long flags;

		spin_lock_irqsave(&func_buf_lock, flags);
		if (func_table[value])
			puts_queue(vc, func_table[value]);
		spin_unlock_irqrestore(&func_buf_lock, flags);

	} else
		pr_err("k_fn called with value=%d\n", value);
}
