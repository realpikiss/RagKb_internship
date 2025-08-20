struct inode *ntfs_iget5(struct super_block *sb, const struct MFT_REF *ref,
			 const struct cpu_str *name)
{
	struct inode *inode;

	inode = iget5_locked(sb, ino_get(ref), ntfs_test_inode, ntfs_set_inode,
			     (void *)ref);
	if (unlikely(!inode))
		return ERR_PTR(-ENOMEM);

	/* If this is a freshly allocated inode, need to read it now. */
	if (inode->i_state & I_NEW)
		inode = ntfs_read_mft(inode, name, ref);
	else if (ref->seq != ntfs_i(inode)->mi.mrec->seq) {
		/* Inode overlaps? */
		_ntfs_bad_inode(inode);
	}

	return inode;
}
