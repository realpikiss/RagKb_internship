asmlinkage int printk(const char *fmt, ...)
{
	va_list args;
	int r;

#ifdef CONFIG_KGDB_KDB
	if (unlikely(kdb_trap_printk)) {
		va_start(args, fmt);
		r = vkdb_printf(fmt, args);
		va_end(args);
		return r;
	}
#endif
	va_start(args, fmt);
	r = vprintk(fmt, args);
	va_end(args);

	return r;
}
