struct rchan *relay_open(const char *base_filename,
			 struct dentry *parent,
			 size_t subbuf_size,
			 size_t n_subbufs,
			 struct rchan_callbacks *cb,
			 void *private_data)
{
	unsigned int i;
	struct rchan *chan;
	struct rchan_buf *buf;

	if (!(subbuf_size && n_subbufs))
		return NULL;
	if (subbuf_size > UINT_MAX / n_subbufs)
		return NULL;

	chan = kzalloc(sizeof(struct rchan), GFP_KERNEL);
	if (!chan)
		return NULL;

	chan->buf = alloc_percpu(struct rchan_buf *);
	if (!chan->buf) {
		kfree(chan);
		return NULL;
	}

	chan->version = RELAYFS_CHANNEL_VERSION;
	chan->n_subbufs = n_subbufs;
	chan->subbuf_size = subbuf_size;
	chan->alloc_size = PAGE_ALIGN(subbuf_size * n_subbufs);
	chan->parent = parent;
	chan->private_data = private_data;
	if (base_filename) {
		chan->has_base_filename = 1;
		strlcpy(chan->base_filename, base_filename, NAME_MAX);
	}
	setup_callbacks(chan, cb);
	kref_init(&chan->kref);

	mutex_lock(&relay_channels_mutex);
	for_each_online_cpu(i) {
		buf = relay_open_buf(chan, i);
		if (!buf)
			goto free_bufs;
		*per_cpu_ptr(chan->buf, i) = buf;
	}
	list_add(&chan->list, &relay_channels);
	mutex_unlock(&relay_channels_mutex);

	return chan;

free_bufs:
	for_each_possible_cpu(i) {
		if ((buf = *per_cpu_ptr(chan->buf, i)))
			relay_close_buf(buf);
	}

	kref_put(&chan->kref, relay_destroy_channel);
	mutex_unlock(&relay_channels_mutex);
	return NULL;
}
