void ext4_es_insert_extent(struct inode *inode, ext4_lblk_t lblk,
			   ext4_lblk_t len, ext4_fsblk_t pblk,
			   unsigned int status)
{
	struct extent_status newes;
	ext4_lblk_t end = lblk + len - 1;
	int err1 = 0;
	int err2 = 0;
	struct ext4_sb_info *sbi = EXT4_SB(inode->i_sb);
	struct extent_status *es1 = NULL;
	struct extent_status *es2 = NULL;

	if (EXT4_SB(inode->i_sb)->s_mount_state & EXT4_FC_REPLAY)
		return;

	es_debug("add [%u/%u) %llu %x to extent status tree of inode %lu\n",
		 lblk, len, pblk, status, inode->i_ino);

	if (!len)
		return;

	BUG_ON(end < lblk);

	if ((status & EXTENT_STATUS_DELAYED) &&
	    (status & EXTENT_STATUS_WRITTEN)) {
		ext4_warning(inode->i_sb, "Inserting extent [%u/%u] as "
				" delayed and written which can potentially "
				" cause data loss.", lblk, len);
		WARN_ON(1);
	}

	newes.es_lblk = lblk;
	newes.es_len = len;
	ext4_es_store_pblock_status(&newes, pblk, status);
	trace_ext4_es_insert_extent(inode, &newes);

	ext4_es_insert_extent_check(inode, &newes);

retry:
	if (err1 && !es1)
		es1 = __es_alloc_extent(true);
	if ((err1 || err2) && !es2)
		es2 = __es_alloc_extent(true);
	write_lock(&EXT4_I(inode)->i_es_lock);

	err1 = __es_remove_extent(inode, lblk, end, NULL, es1);
	if (err1 != 0)
		goto error;

	err2 = __es_insert_extent(inode, &newes, es2);
	if (err2 == -ENOMEM && !ext4_es_must_keep(&newes))
		err2 = 0;
	if (err2 != 0)
		goto error;

	if (sbi->s_cluster_ratio > 1 && test_opt(inode->i_sb, DELALLOC) &&
	    (status & EXTENT_STATUS_WRITTEN ||
	     status & EXTENT_STATUS_UNWRITTEN))
		__revise_pending(inode, lblk, len);

	/* es is pre-allocated but not used, free it. */
	if (es1 && !es1->es_len)
		__es_free_extent(es1);
	if (es2 && !es2->es_len)
		__es_free_extent(es2);
error:
	write_unlock(&EXT4_I(inode)->i_es_lock);
	if (err1 || err2)
		goto retry;

	ext4_es_print_tree(inode);
	return;
}
