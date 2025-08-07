int ext4_xattr_ibody_find(struct inode *inode, struct ext4_xattr_info *i,
			  struct ext4_xattr_ibody_find *is)
{
	struct ext4_xattr_ibody_header *header;
	struct ext4_inode *raw_inode;
	int error;

	if (EXT4_I(inode)->i_extra_isize == 0)
		return 0;
	raw_inode = ext4_raw_inode(&is->iloc);
	header = IHDR(inode, raw_inode);
	is->s.base = is->s.first = IFIRST(header);
	is->s.here = is->s.first;
	is->s.end = (void *)raw_inode + EXT4_SB(inode->i_sb)->s_inode_size;
	if (ext4_test_inode_state(inode, EXT4_STATE_XATTR)) {
		error = xattr_check_inode(inode, header, is->s.end);
		if (error)
			return error;
		/* Find the named attribute. */
		error = xattr_find_entry(inode, &is->s.here, is->s.end,
					 i->name_index, i->name, 0);
		if (error && error != -ENODATA)
			return error;
		is->s.not_found = error;
	}
	return 0;
}