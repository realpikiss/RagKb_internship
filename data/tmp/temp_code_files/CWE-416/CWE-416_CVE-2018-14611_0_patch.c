static int btrfs_check_chunk_valid(struct btrfs_fs_info *fs_info,
				   struct extent_buffer *leaf,
				   struct btrfs_chunk *chunk, u64 logical)
{
	u64 length;
	u64 stripe_len;
	u16 num_stripes;
	u16 sub_stripes;
	u64 type;
	u64 features;
	bool mixed = false;

	length = btrfs_chunk_length(leaf, chunk);
	stripe_len = btrfs_chunk_stripe_len(leaf, chunk);
	num_stripes = btrfs_chunk_num_stripes(leaf, chunk);
	sub_stripes = btrfs_chunk_sub_stripes(leaf, chunk);
	type = btrfs_chunk_type(leaf, chunk);

	if (!num_stripes) {
		btrfs_err(fs_info, "invalid chunk num_stripes: %u",
			  num_stripes);
		return -EIO;
	}
	if (!IS_ALIGNED(logical, fs_info->sectorsize)) {
		btrfs_err(fs_info, "invalid chunk logical %llu", logical);
		return -EIO;
	}
	if (btrfs_chunk_sector_size(leaf, chunk) != fs_info->sectorsize) {
		btrfs_err(fs_info, "invalid chunk sectorsize %u",
			  btrfs_chunk_sector_size(leaf, chunk));
		return -EIO;
	}
	if (!length || !IS_ALIGNED(length, fs_info->sectorsize)) {
		btrfs_err(fs_info, "invalid chunk length %llu", length);
		return -EIO;
	}
	if (!is_power_of_2(stripe_len) || stripe_len != BTRFS_STRIPE_LEN) {
		btrfs_err(fs_info, "invalid chunk stripe length: %llu",
			  stripe_len);
		return -EIO;
	}
	if (~(BTRFS_BLOCK_GROUP_TYPE_MASK | BTRFS_BLOCK_GROUP_PROFILE_MASK) &
	    type) {
		btrfs_err(fs_info, "unrecognized chunk type: %llu",
			  ~(BTRFS_BLOCK_GROUP_TYPE_MASK |
			    BTRFS_BLOCK_GROUP_PROFILE_MASK) &
			  btrfs_chunk_type(leaf, chunk));
		return -EIO;
	}

	if ((type & BTRFS_BLOCK_GROUP_TYPE_MASK) == 0) {
		btrfs_err(fs_info, "missing chunk type flag: 0x%llx", type);
		return -EIO;
	}

	if ((type & BTRFS_BLOCK_GROUP_SYSTEM) &&
	    (type & (BTRFS_BLOCK_GROUP_METADATA | BTRFS_BLOCK_GROUP_DATA))) {
		btrfs_err(fs_info,
			"system chunk with data or metadata type: 0x%llx", type);
		return -EIO;
	}

	features = btrfs_super_incompat_flags(fs_info->super_copy);
	if (features & BTRFS_FEATURE_INCOMPAT_MIXED_GROUPS)
		mixed = true;

	if (!mixed) {
		if ((type & BTRFS_BLOCK_GROUP_METADATA) &&
		    (type & BTRFS_BLOCK_GROUP_DATA)) {
			btrfs_err(fs_info,
			"mixed chunk type in non-mixed mode: 0x%llx", type);
			return -EIO;
		}
	}

	if ((type & BTRFS_BLOCK_GROUP_RAID10 && sub_stripes != 2) ||
	    (type & BTRFS_BLOCK_GROUP_RAID1 && num_stripes < 1) ||
	    (type & BTRFS_BLOCK_GROUP_RAID5 && num_stripes < 2) ||
	    (type & BTRFS_BLOCK_GROUP_RAID6 && num_stripes < 3) ||
	    (type & BTRFS_BLOCK_GROUP_DUP && num_stripes > 2) ||
	    ((type & BTRFS_BLOCK_GROUP_PROFILE_MASK) == 0 &&
	     num_stripes != 1)) {
		btrfs_err(fs_info,
			"invalid num_stripes:sub_stripes %u:%u for profile %llu",
			num_stripes, sub_stripes,
			type & BTRFS_BLOCK_GROUP_PROFILE_MASK);
		return -EIO;
	}

	return 0;
}