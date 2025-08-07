static struct mountpoint *get_mountpoint(struct dentry *dentry)
{
	struct mountpoint *mp, *new = NULL;
	int ret;

	if (d_mountpoint(dentry)) {
		/* might be worth a WARN_ON() */
		if (d_unlinked(dentry))
			return ERR_PTR(-ENOENT);
mountpoint:
		read_seqlock_excl(&mount_lock);
		mp = lookup_mountpoint(dentry);
		read_sequnlock_excl(&mount_lock);
		if (mp)
			goto done;
	}

	if (!new)
		new = kmalloc(sizeof(struct mountpoint), GFP_KERNEL);
	if (!new)
		return ERR_PTR(-ENOMEM);


	/* Exactly one processes may set d_mounted */
	ret = d_set_mounted(dentry);

	/* Someone else set d_mounted? */
	if (ret == -EBUSY)
		goto mountpoint;

	/* The dentry is not available as a mountpoint? */
	mp = ERR_PTR(ret);
	if (ret)
		goto done;

	/* Add the new mountpoint to the hash table */
	read_seqlock_excl(&mount_lock);
	new->m_dentry = dget(dentry);
	new->m_count = 1;
	hlist_add_head(&new->m_hash, mp_hash(dentry));
	INIT_HLIST_HEAD(&new->m_list);
	read_sequnlock_excl(&mount_lock);

	mp = new;
	new = NULL;
done:
	kfree(new);
	return mp;
}