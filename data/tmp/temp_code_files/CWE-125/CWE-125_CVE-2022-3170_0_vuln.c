static unsigned long get_ctl_id_hash(const struct snd_ctl_elem_id *id)
{
	unsigned long h;
	const unsigned char *p;

	h = id->iface;
	h = MULTIPLIER * h + id->device;
	h = MULTIPLIER * h + id->subdevice;
	for (p = id->name; *p; p++)
		h = MULTIPLIER * h + *p;
	h = MULTIPLIER * h + id->index;
	h &= LONG_MAX;
	return h;
}