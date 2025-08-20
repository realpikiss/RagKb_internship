int prepare_to_relocate(struct reloc_control *rc)
{
	struct btrfs_trans_handle *trans;
	int ret;

	rc->block_rsv = btrfs_alloc_block_rsv(rc->extent_root->fs_info,
					      BTRFS_BLOCK_RSV_TEMP);
	if (!rc->block_rsv)
		return -ENOMEM;

	memset(&rc->cluster, 0, sizeof(rc->cluster));
	rc->search_start = rc->block_group->start;
	rc->extents_found = 0;
	rc->nodes_relocated = 0;
	rc->merging_rsv_size = 0;
	rc->reserved_bytes = 0;
	rc->block_rsv->size = rc->extent_root->fs_info->nodesize *
			      RELOCATION_RESERVED_NODES;
	ret = btrfs_block_rsv_refill(rc->extent_root->fs_info,
				     rc->block_rsv, rc->block_rsv->size,
				     BTRFS_RESERVE_FLUSH_ALL);
	if (ret)
		return ret;

	rc->create_reloc_tree = 1;
	set_reloc_control(rc);

	trans = btrfs_join_transaction(rc->extent_root);
	if (IS_ERR(trans)) {
		unset_reloc_control(rc);
		/*
		 * extent tree is not a ref_cow tree and has no reloc_root to
		 * cleanup.  And callers are responsible to free the above
		 * block rsv.
		 */
		return PTR_ERR(trans);
	}

	ret = btrfs_commit_transaction(trans);
	if (ret)
		unset_reloc_control(rc);

	return ret;
}
