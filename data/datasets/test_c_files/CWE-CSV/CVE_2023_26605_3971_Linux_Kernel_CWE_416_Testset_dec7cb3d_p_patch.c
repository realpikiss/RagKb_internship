static int writeback_single_inode(struct inode *inode,
				  struct writeback_control *wbc)
{
	struct bdi_writeback *wb;
	int ret = 0;

	spin_lock(&inode->i_lock);
	if (!atomic_read(&inode->i_count))
		WARN_ON(!(inode->i_state & (I_WILL_FREE|I_FREEING)));
	else
		WARN_ON(inode->i_state & I_WILL_FREE);

	if (inode->i_state & I_SYNC) {
		/*
		 * Writeback is already running on the inode.  For WB_SYNC_NONE,
		 * that's enough and we can just return.  For WB_SYNC_ALL, we
		 * must wait for the existing writeback to complete, then do
		 * writeback again if there's anything left.
		 */
		if (wbc->sync_mode != WB_SYNC_ALL)
			goto out;
		__inode_wait_for_writeback(inode);
	}
	WARN_ON(inode->i_state & I_SYNC);
	/*
	 * If the inode is already fully clean, then there's nothing to do.
	 *
	 * For data-integrity syncs we also need to check whether any pages are
	 * still under writeback, e.g. due to prior WB_SYNC_NONE writeback.  If
	 * there are any such pages, we'll need to wait for them.
	 */
	if (!(inode->i_state & I_DIRTY_ALL) &&
	    (wbc->sync_mode != WB_SYNC_ALL ||
	     !mapping_tagged(inode->i_mapping, PAGECACHE_TAG_WRITEBACK)))
		goto out;
	inode->i_state |= I_SYNC;
	wbc_attach_and_unlock_inode(wbc, inode);

	ret = __writeback_single_inode(inode, wbc);

	wbc_detach_inode(wbc);

	wb = inode_to_wb_and_lock_list(inode);
	spin_lock(&inode->i_lock);
	/*
	 * If the inode is freeing, its i_io_list shoudn't be updated
	 * as it can be finally deleted at this moment.
	 */
	if (!(inode->i_state & I_FREEING)) {
		/*
		 * If the inode is now fully clean, then it can be safely
		 * removed from its writeback list (if any). Otherwise the
		 * flusher threads are responsible for the writeback lists.
		 */
		if (!(inode->i_state & I_DIRTY_ALL))
			inode_cgwb_move_to_attached(inode, wb);
		else if (!(inode->i_state & I_SYNC_QUEUED)) {
			if ((inode->i_state & I_DIRTY))
				redirty_tail_locked(inode, wb);
			else if (inode->i_state & I_DIRTY_TIME) {
				inode->dirtied_when = jiffies;
				inode_io_list_move_locked(inode,
							  wb,
							  &wb->b_dirty_time);
			}
		}
	}

	spin_unlock(&wb->list_lock);
	inode_sync_complete(inode);
out:
	spin_unlock(&inode->i_lock);
	return ret;
}
