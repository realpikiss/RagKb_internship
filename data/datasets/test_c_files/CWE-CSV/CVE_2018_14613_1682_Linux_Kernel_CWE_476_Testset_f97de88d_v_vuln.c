static int check_leaf_item(struct btrfs_fs_info *fs_info,
			   struct extent_buffer *leaf,
			   struct btrfs_key *key, int slot)
{
	int ret = 0;

	switch (key->type) {
	case BTRFS_EXTENT_DATA_KEY:
		ret = check_extent_data_item(fs_info, leaf, key, slot);
		break;
	case BTRFS_EXTENT_CSUM_KEY:
		ret = check_csum_item(fs_info, leaf, key, slot);
		break;
	case BTRFS_DIR_ITEM_KEY:
	case BTRFS_DIR_INDEX_KEY:
	case BTRFS_XATTR_ITEM_KEY:
		ret = check_dir_item(fs_info, leaf, key, slot);
		break;
	}
	return ret;
}
