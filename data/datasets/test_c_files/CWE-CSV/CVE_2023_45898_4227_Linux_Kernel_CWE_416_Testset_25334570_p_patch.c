void ext4_es_insert_delayed_block(struct inode *inode, ext4_lblk_t lblk,
				  bool allocated)
{
	struct extent_status newes;
	int err1 = 0;
	int err2 = 0;
	struct extent_status *es1 = NULL;
	struct extent_status *es2 = NULL;

	if (EXT4_SB(inode->i_sb)->s_mount_state & EXT4_FC_REPLAY)
		return;

	es_debug("add [%u/1) delayed to extent status tree of inode %lu\n",
		 lblk, inode->i_ino);

	newes.es_lblk = lblk;
	newes.es_len = 1;
	ext4_es_store_pblock_status(&newes, ~0, EXTENT_STATUS_DELAYED);
	trace_ext4_es_insert_delayed_block(inode, &newes, allocated);

	ext4_es_insert_extent_check(inode, &newes);

retry:
	if (err1 && !es1)
		es1 = __es_alloc_extent(true);
	if ((err1 || err2) && !es2)
		es2 = __es_alloc_extent(true);
	write_lock(&EXT4_I(inode)->i_es_lock);

	err1 = __es_remove_extent(inode, lblk, lblk, NULL, es1);
	if (err1 != 0)
		goto error;
	/* Free preallocated extent if it didn't get used. */
	if (es1) {
		if (!es1->es_len)
			__es_free_extent(es1);
		es1 = NULL;
	}

	err2 = __es_insert_extent(inode, &newes, es2);
	if (err2 != 0)
		goto error;
	/* Free preallocated extent if it didn't get used. */
	if (es2) {
		if (!es2->es_len)
			__es_free_extent(es2);
		es2 = NULL;
	}

	if (allocated)
		__insert_pending(inode, lblk);
error:
	write_unlock(&EXT4_I(inode)->i_es_lock);
	if (err1 || err2)
		goto retry;

	ext4_es_print_tree(inode);
	ext4_print_pending_tree(inode);
	return;
}
