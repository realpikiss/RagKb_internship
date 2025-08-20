int attr_punch_hole(struct ntfs_inode *ni, u64 vbo, u64 bytes, u32 *frame_size)
{
	int err = 0;
	struct runs_tree *run = &ni->file.run;
	struct ntfs_sb_info *sbi = ni->mi.sbi;
	struct ATTRIB *attr = NULL, *attr_b;
	struct ATTR_LIST_ENTRY *le, *le_b;
	struct mft_inode *mi, *mi_b;
	CLST svcn, evcn1, vcn, len, end, alen, hole, next_svcn;
	u64 total_size, alloc_size;
	u32 mask;
	__le16 a_flags;
	struct runs_tree run2;

	if (!bytes)
		return 0;

	le_b = NULL;
	attr_b = ni_find_attr(ni, NULL, &le_b, ATTR_DATA, NULL, 0, NULL, &mi_b);
	if (!attr_b)
		return -ENOENT;

	if (!attr_b->non_res) {
		u32 data_size = le32_to_cpu(attr_b->res.data_size);
		u32 from, to;

		if (vbo > data_size)
			return 0;

		from = vbo;
		to = min_t(u64, vbo + bytes, data_size);
		memset(Add2Ptr(resident_data(attr_b), from), 0, to - from);
		return 0;
	}

	if (!is_attr_ext(attr_b))
		return -EOPNOTSUPP;

	alloc_size = le64_to_cpu(attr_b->nres.alloc_size);
	total_size = le64_to_cpu(attr_b->nres.total_size);

	if (vbo >= alloc_size) {
		/* NOTE: It is allowed. */
		return 0;
	}

	mask = (sbi->cluster_size << attr_b->nres.c_unit) - 1;

	bytes += vbo;
	if (bytes > alloc_size)
		bytes = alloc_size;
	bytes -= vbo;

	if ((vbo & mask) || (bytes & mask)) {
		/* We have to zero a range(s). */
		if (frame_size == NULL) {
			/* Caller insists range is aligned. */
			return -EINVAL;
		}
		*frame_size = mask + 1;
		return E_NTFS_NOTALIGNED;
	}

	down_write(&ni->file.run_lock);
	run_init(&run2);
	run_truncate(run, 0);

	/*
	 * Enumerate all attribute segments and punch hole where necessary.
	 */
	alen = alloc_size >> sbi->cluster_bits;
	vcn = vbo >> sbi->cluster_bits;
	len = bytes >> sbi->cluster_bits;
	end = vcn + len;
	hole = 0;

	svcn = le64_to_cpu(attr_b->nres.svcn);
	evcn1 = le64_to_cpu(attr_b->nres.evcn) + 1;
	a_flags = attr_b->flags;

	if (svcn <= vcn && vcn < evcn1) {
		attr = attr_b;
		le = le_b;
		mi = mi_b;
	} else if (!le_b) {
		err = -EINVAL;
		goto bad_inode;
	} else {
		le = le_b;
		attr = ni_find_attr(ni, attr_b, &le, ATTR_DATA, NULL, 0, &vcn,
				    &mi);
		if (!attr) {
			err = -EINVAL;
			goto bad_inode;
		}

		svcn = le64_to_cpu(attr->nres.svcn);
		evcn1 = le64_to_cpu(attr->nres.evcn) + 1;
	}

	while (svcn < end) {
		CLST vcn1, zero, hole2 = hole;

		err = attr_load_runs(attr, ni, run, &svcn);
		if (err)
			goto done;
		vcn1 = max(vcn, svcn);
		zero = min(end, evcn1) - vcn1;

		/*
		 * Check range [vcn1 + zero).
		 * Calculate how many clusters there are.
		 * Don't do any destructive actions.
		 */
		err = run_deallocate_ex(NULL, run, vcn1, zero, &hole2, false);
		if (err)
			goto done;

		/* Check if required range is already hole. */
		if (hole2 == hole)
			goto next_attr;

		/* Make a clone of run to undo. */
		err = run_clone(run, &run2);
		if (err)
			goto done;

		/* Make a hole range (sparse) [vcn1 + zero). */
		if (!run_add_entry(run, vcn1, SPARSE_LCN, zero, false)) {
			err = -ENOMEM;
			goto done;
		}

		/* Update run in attribute segment. */
		err = mi_pack_runs(mi, attr, run, evcn1 - svcn);
		if (err)
			goto done;
		next_svcn = le64_to_cpu(attr->nres.evcn) + 1;
		if (next_svcn < evcn1) {
			/* Insert new attribute segment. */
			err = ni_insert_nonresident(ni, ATTR_DATA, NULL, 0, run,
						    next_svcn,
						    evcn1 - next_svcn, a_flags,
						    &attr, &mi, &le);
			if (err)
				goto undo_punch;

			/* Layout of records maybe changed. */
			attr_b = NULL;
		}

		/* Real deallocate. Should not fail. */
		run_deallocate_ex(sbi, &run2, vcn1, zero, &hole, true);

next_attr:
		/* Free all allocated memory. */
		run_truncate(run, 0);

		if (evcn1 >= alen)
			break;

		/* Get next attribute segment. */
		attr = ni_enum_attr_ex(ni, attr, &le, &mi);
		if (!attr) {
			err = -EINVAL;
			goto bad_inode;
		}

		svcn = le64_to_cpu(attr->nres.svcn);
		evcn1 = le64_to_cpu(attr->nres.evcn) + 1;
	}

done:
	if (!hole)
		goto out;

	if (!attr_b) {
		attr_b = ni_find_attr(ni, NULL, NULL, ATTR_DATA, NULL, 0, NULL,
				      &mi_b);
		if (!attr_b) {
			err = -EINVAL;
			goto bad_inode;
		}
	}

	total_size -= (u64)hole << sbi->cluster_bits;
	attr_b->nres.total_size = cpu_to_le64(total_size);
	mi_b->dirty = true;

	/* Update inode size. */
	inode_set_bytes(&ni->vfs_inode, total_size);
	ni->ni_flags |= NI_FLAG_UPDATE_PARENT;
	mark_inode_dirty(&ni->vfs_inode);

out:
	run_close(&run2);
	up_write(&ni->file.run_lock);
	return err;

bad_inode:
	_ntfs_bad_inode(&ni->vfs_inode);
	goto out;

undo_punch:
	/*
	 * Restore packed runs.
	 * 'mi_pack_runs' should not fail, cause we restore original.
	 */
	if (mi_pack_runs(mi, attr, &run2, evcn1 - svcn))
		goto bad_inode;

	goto done;
}
