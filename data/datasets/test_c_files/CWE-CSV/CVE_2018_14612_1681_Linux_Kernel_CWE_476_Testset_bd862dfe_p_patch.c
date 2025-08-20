static int check_leaf(struct btrfs_fs_info *fs_info, struct extent_buffer *leaf,
		      bool check_item_data)
{
	/* No valid key type is 0, so all key should be larger than this key */
	struct btrfs_key prev_key = {0, 0, 0};
	struct btrfs_key key;
	u32 nritems = btrfs_header_nritems(leaf);
	int slot;

	/*
	 * Extent buffers from a relocation tree have a owner field that
	 * corresponds to the subvolume tree they are based on. So just from an
	 * extent buffer alone we can not find out what is the id of the
	 * corresponding subvolume tree, so we can not figure out if the extent
	 * buffer corresponds to the root of the relocation tree or not. So
	 * skip this check for relocation trees.
	 */
	if (nritems == 0 && !btrfs_header_flag(leaf, BTRFS_HEADER_FLAG_RELOC)) {
		u64 owner = btrfs_header_owner(leaf);
		struct btrfs_root *check_root;

		/* These trees must never be empty */
		if (owner == BTRFS_ROOT_TREE_OBJECTID ||
		    owner == BTRFS_CHUNK_TREE_OBJECTID ||
		    owner == BTRFS_EXTENT_TREE_OBJECTID ||
		    owner == BTRFS_DEV_TREE_OBJECTID ||
		    owner == BTRFS_FS_TREE_OBJECTID ||
		    owner == BTRFS_DATA_RELOC_TREE_OBJECTID) {
			generic_err(fs_info, leaf, 0,
			"invalid root, root %llu must never be empty",
				    owner);
			return -EUCLEAN;
		}
		key.objectid = owner;
		key.type = BTRFS_ROOT_ITEM_KEY;
		key.offset = (u64)-1;

		check_root = btrfs_get_fs_root(fs_info, &key, false);
		/*
		 * The only reason we also check NULL here is that during
		 * open_ctree() some roots has not yet been set up.
		 */
		if (!IS_ERR_OR_NULL(check_root)) {
			struct extent_buffer *eb;

			eb = btrfs_root_node(check_root);
			/* if leaf is the root, then it's fine */
			if (leaf != eb) {
				generic_err(fs_info, leaf, 0,
		"invalid nritems, have %u should not be 0 for non-root leaf",
					nritems);
				free_extent_buffer(eb);
				return -EUCLEAN;
			}
			free_extent_buffer(eb);
		}
		return 0;
	}

	if (nritems == 0)
		return 0;

	/*
	 * Check the following things to make sure this is a good leaf, and
	 * leaf users won't need to bother with similar sanity checks:
	 *
	 * 1) key ordering
	 * 2) item offset and size
	 *    No overlap, no hole, all inside the leaf.
	 * 3) item content
	 *    If possible, do comprehensive sanity check.
	 *    NOTE: All checks must only rely on the item data itself.
	 */
	for (slot = 0; slot < nritems; slot++) {
		u32 item_end_expected;
		int ret;

		btrfs_item_key_to_cpu(leaf, &key, slot);

		/* Make sure the keys are in the right order */
		if (btrfs_comp_cpu_keys(&prev_key, &key) >= 0) {
			generic_err(fs_info, leaf, slot,
	"bad key order, prev (%llu %u %llu) current (%llu %u %llu)",
				prev_key.objectid, prev_key.type,
				prev_key.offset, key.objectid, key.type,
				key.offset);
			return -EUCLEAN;
		}

		/*
		 * Make sure the offset and ends are right, remember that the
		 * item data starts at the end of the leaf and grows towards the
		 * front.
		 */
		if (slot == 0)
			item_end_expected = BTRFS_LEAF_DATA_SIZE(fs_info);
		else
			item_end_expected = btrfs_item_offset_nr(leaf,
								 slot - 1);
		if (btrfs_item_end_nr(leaf, slot) != item_end_expected) {
			generic_err(fs_info, leaf, slot,
				"unexpected item end, have %u expect %u",
				btrfs_item_end_nr(leaf, slot),
				item_end_expected);
			return -EUCLEAN;
		}

		/*
		 * Check to make sure that we don't point outside of the leaf,
		 * just in case all the items are consistent to each other, but
		 * all point outside of the leaf.
		 */
		if (btrfs_item_end_nr(leaf, slot) >
		    BTRFS_LEAF_DATA_SIZE(fs_info)) {
			generic_err(fs_info, leaf, slot,
			"slot end outside of leaf, have %u expect range [0, %u]",
				btrfs_item_end_nr(leaf, slot),
				BTRFS_LEAF_DATA_SIZE(fs_info));
			return -EUCLEAN;
		}

		/* Also check if the item pointer overlaps with btrfs item. */
		if (btrfs_item_nr_offset(slot) + sizeof(struct btrfs_item) >
		    btrfs_item_ptr_offset(leaf, slot)) {
			generic_err(fs_info, leaf, slot,
		"slot overlaps with its data, item end %lu data start %lu",
				btrfs_item_nr_offset(slot) +
				sizeof(struct btrfs_item),
				btrfs_item_ptr_offset(leaf, slot));
			return -EUCLEAN;
		}

		if (check_item_data) {
			/*
			 * Check if the item size and content meet other
			 * criteria
			 */
			ret = check_leaf_item(fs_info, leaf, &key, slot);
			if (ret < 0)
				return ret;
		}

		prev_key.objectid = key.objectid;
		prev_key.type = key.type;
		prev_key.offset = key.offset;
	}

	return 0;
}
