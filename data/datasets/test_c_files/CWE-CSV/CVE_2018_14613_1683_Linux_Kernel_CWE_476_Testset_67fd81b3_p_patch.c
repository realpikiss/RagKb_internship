static int __btrfs_alloc_chunk(struct btrfs_trans_handle *trans,
			       u64 start, u64 type)
{
	struct btrfs_fs_info *info = trans->fs_info;
	struct btrfs_fs_devices *fs_devices = info->fs_devices;
	struct btrfs_device *device;
	struct map_lookup *map = NULL;
	struct extent_map_tree *em_tree;
	struct extent_map *em;
	struct btrfs_device_info *devices_info = NULL;
	u64 total_avail;
	int num_stripes;	/* total number of stripes to allocate */
	int data_stripes;	/* number of stripes that count for
				   block group size */
	int sub_stripes;	/* sub_stripes info for map */
	int dev_stripes;	/* stripes per dev */
	int devs_max;		/* max devs to use */
	int devs_min;		/* min devs needed */
	int devs_increment;	/* ndevs has to be a multiple of this */
	int ncopies;		/* how many copies to data has */
	int ret;
	u64 max_stripe_size;
	u64 max_chunk_size;
	u64 stripe_size;
	u64 num_bytes;
	int ndevs;
	int i;
	int j;
	int index;

	BUG_ON(!alloc_profile_is_valid(type, 0));

	if (list_empty(&fs_devices->alloc_list)) {
		if (btrfs_test_opt(info, ENOSPC_DEBUG))
			btrfs_debug(info, "%s: no writable device", __func__);
		return -ENOSPC;
	}

	index = btrfs_bg_flags_to_raid_index(type);

	sub_stripes = btrfs_raid_array[index].sub_stripes;
	dev_stripes = btrfs_raid_array[index].dev_stripes;
	devs_max = btrfs_raid_array[index].devs_max;
	devs_min = btrfs_raid_array[index].devs_min;
	devs_increment = btrfs_raid_array[index].devs_increment;
	ncopies = btrfs_raid_array[index].ncopies;

	if (type & BTRFS_BLOCK_GROUP_DATA) {
		max_stripe_size = SZ_1G;
		max_chunk_size = BTRFS_MAX_DATA_CHUNK_SIZE;
		if (!devs_max)
			devs_max = BTRFS_MAX_DEVS(info);
	} else if (type & BTRFS_BLOCK_GROUP_METADATA) {
		/* for larger filesystems, use larger metadata chunks */
		if (fs_devices->total_rw_bytes > 50ULL * SZ_1G)
			max_stripe_size = SZ_1G;
		else
			max_stripe_size = SZ_256M;
		max_chunk_size = max_stripe_size;
		if (!devs_max)
			devs_max = BTRFS_MAX_DEVS(info);
	} else if (type & BTRFS_BLOCK_GROUP_SYSTEM) {
		max_stripe_size = SZ_32M;
		max_chunk_size = 2 * max_stripe_size;
		if (!devs_max)
			devs_max = BTRFS_MAX_DEVS_SYS_CHUNK;
	} else {
		btrfs_err(info, "invalid chunk type 0x%llx requested",
		       type);
		BUG_ON(1);
	}

	/* we don't want a chunk larger than 10% of writeable space */
	max_chunk_size = min(div_factor(fs_devices->total_rw_bytes, 1),
			     max_chunk_size);

	devices_info = kcalloc(fs_devices->rw_devices, sizeof(*devices_info),
			       GFP_NOFS);
	if (!devices_info)
		return -ENOMEM;

	/*
	 * in the first pass through the devices list, we gather information
	 * about the available holes on each device.
	 */
	ndevs = 0;
	list_for_each_entry(device, &fs_devices->alloc_list, dev_alloc_list) {
		u64 max_avail;
		u64 dev_offset;

		if (!test_bit(BTRFS_DEV_STATE_WRITEABLE, &device->dev_state)) {
			WARN(1, KERN_ERR
			       "BTRFS: read-only device in alloc_list\n");
			continue;
		}

		if (!test_bit(BTRFS_DEV_STATE_IN_FS_METADATA,
					&device->dev_state) ||
		    test_bit(BTRFS_DEV_STATE_REPLACE_TGT, &device->dev_state))
			continue;

		if (device->total_bytes > device->bytes_used)
			total_avail = device->total_bytes - device->bytes_used;
		else
			total_avail = 0;

		/* If there is no space on this device, skip it. */
		if (total_avail == 0)
			continue;

		ret = find_free_dev_extent(trans, device,
					   max_stripe_size * dev_stripes,
					   &dev_offset, &max_avail);
		if (ret && ret != -ENOSPC)
			goto error;

		if (ret == 0)
			max_avail = max_stripe_size * dev_stripes;

		if (max_avail < BTRFS_STRIPE_LEN * dev_stripes) {
			if (btrfs_test_opt(info, ENOSPC_DEBUG))
				btrfs_debug(info,
			"%s: devid %llu has no free space, have=%llu want=%u",
					    __func__, device->devid, max_avail,
					    BTRFS_STRIPE_LEN * dev_stripes);
			continue;
		}

		if (ndevs == fs_devices->rw_devices) {
			WARN(1, "%s: found more than %llu devices\n",
			     __func__, fs_devices->rw_devices);
			break;
		}
		devices_info[ndevs].dev_offset = dev_offset;
		devices_info[ndevs].max_avail = max_avail;
		devices_info[ndevs].total_avail = total_avail;
		devices_info[ndevs].dev = device;
		++ndevs;
	}

	/*
	 * now sort the devices by hole size / available space
	 */
	sort(devices_info, ndevs, sizeof(struct btrfs_device_info),
	     btrfs_cmp_device_info, NULL);

	/* round down to number of usable stripes */
	ndevs = round_down(ndevs, devs_increment);

	if (ndevs < devs_min) {
		ret = -ENOSPC;
		if (btrfs_test_opt(info, ENOSPC_DEBUG)) {
			btrfs_debug(info,
	"%s: not enough devices with free space: have=%d minimum required=%d",
				    __func__, ndevs, devs_min);
		}
		goto error;
	}

	ndevs = min(ndevs, devs_max);

	/*
	 * The primary goal is to maximize the number of stripes, so use as
	 * many devices as possible, even if the stripes are not maximum sized.
	 *
	 * The DUP profile stores more than one stripe per device, the
	 * max_avail is the total size so we have to adjust.
	 */
	stripe_size = div_u64(devices_info[ndevs - 1].max_avail, dev_stripes);
	num_stripes = ndevs * dev_stripes;

	/*
	 * this will have to be fixed for RAID1 and RAID10 over
	 * more drives
	 */
	data_stripes = num_stripes / ncopies;

	if (type & BTRFS_BLOCK_GROUP_RAID5)
		data_stripes = num_stripes - 1;

	if (type & BTRFS_BLOCK_GROUP_RAID6)
		data_stripes = num_stripes - 2;

	/*
	 * Use the number of data stripes to figure out how big this chunk
	 * is really going to be in terms of logical address space,
	 * and compare that answer with the max chunk size
	 */
	if (stripe_size * data_stripes > max_chunk_size) {
		stripe_size = div_u64(max_chunk_size, data_stripes);

		/* bump the answer up to a 16MB boundary */
		stripe_size = round_up(stripe_size, SZ_16M);

		/*
		 * But don't go higher than the limits we found while searching
		 * for free extents
		 */
		stripe_size = min(devices_info[ndevs - 1].max_avail,
				  stripe_size);
	}

	/* align to BTRFS_STRIPE_LEN */
	stripe_size = round_down(stripe_size, BTRFS_STRIPE_LEN);

	map = kmalloc(map_lookup_size(num_stripes), GFP_NOFS);
	if (!map) {
		ret = -ENOMEM;
		goto error;
	}
	map->num_stripes = num_stripes;

	for (i = 0; i < ndevs; ++i) {
		for (j = 0; j < dev_stripes; ++j) {
			int s = i * dev_stripes + j;
			map->stripes[s].dev = devices_info[i].dev;
			map->stripes[s].physical = devices_info[i].dev_offset +
						   j * stripe_size;
		}
	}
	map->stripe_len = BTRFS_STRIPE_LEN;
	map->io_align = BTRFS_STRIPE_LEN;
	map->io_width = BTRFS_STRIPE_LEN;
	map->type = type;
	map->sub_stripes = sub_stripes;

	num_bytes = stripe_size * data_stripes;

	trace_btrfs_chunk_alloc(info, map, start, num_bytes);

	em = alloc_extent_map();
	if (!em) {
		kfree(map);
		ret = -ENOMEM;
		goto error;
	}
	set_bit(EXTENT_FLAG_FS_MAPPING, &em->flags);
	em->map_lookup = map;
	em->start = start;
	em->len = num_bytes;
	em->block_start = 0;
	em->block_len = em->len;
	em->orig_block_len = stripe_size;

	em_tree = &info->mapping_tree.map_tree;
	write_lock(&em_tree->lock);
	ret = add_extent_mapping(em_tree, em, 0);
	if (ret) {
		write_unlock(&em_tree->lock);
		free_extent_map(em);
		goto error;
	}

	list_add_tail(&em->list, &trans->transaction->pending_chunks);
	refcount_inc(&em->refs);
	write_unlock(&em_tree->lock);

	ret = btrfs_make_block_group(trans, 0, type, start, num_bytes);
	if (ret)
		goto error_del_extent;

	for (i = 0; i < map->num_stripes; i++) {
		num_bytes = map->stripes[i].dev->bytes_used + stripe_size;
		btrfs_device_set_bytes_used(map->stripes[i].dev, num_bytes);
	}

	atomic64_sub(stripe_size * map->num_stripes, &info->free_chunk_space);

	free_extent_map(em);
	check_raid56_incompat_flag(info, type);

	kfree(devices_info);
	return 0;

error_del_extent:
	write_lock(&em_tree->lock);
	remove_extent_mapping(em_tree, em);
	write_unlock(&em_tree->lock);

	/* One for our allocation */
	free_extent_map(em);
	/* One for the tree reference */
	free_extent_map(em);
	/* One for the pending_chunks list reference */
	free_extent_map(em);
error:
	kfree(devices_info);
	return ret;
}
