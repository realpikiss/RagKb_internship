static int ntfs_read_ea(struct ntfs_inode *ni, struct EA_FULL **ea,
			size_t add_bytes, const struct EA_INFO **info)
{
	int err;
	struct ntfs_sb_info *sbi = ni->mi.sbi;
	struct ATTR_LIST_ENTRY *le = NULL;
	struct ATTRIB *attr_info, *attr_ea;
	void *ea_p;
	u32 size;

	static_assert(le32_to_cpu(ATTR_EA_INFO) < le32_to_cpu(ATTR_EA));

	*ea = NULL;
	*info = NULL;

	attr_info =
		ni_find_attr(ni, NULL, &le, ATTR_EA_INFO, NULL, 0, NULL, NULL);
	attr_ea =
		ni_find_attr(ni, attr_info, &le, ATTR_EA, NULL, 0, NULL, NULL);

	if (!attr_ea || !attr_info)
		return 0;

	*info = resident_data_ex(attr_info, sizeof(struct EA_INFO));
	if (!*info)
		return -EINVAL;

	/* Check Ea limit. */
	size = le32_to_cpu((*info)->size);
	if (size > sbi->ea_max_size)
		return -EFBIG;

	if (attr_size(attr_ea) > sbi->ea_max_size)
		return -EFBIG;

	/* Allocate memory for packed Ea. */
	ea_p = kmalloc(size_add(size, add_bytes), GFP_NOFS);
	if (!ea_p)
		return -ENOMEM;

	if (!size) {
		/* EA info persists, but xattr is empty. Looks like EA problem. */
	} else if (attr_ea->non_res) {
		struct runs_tree run;

		run_init(&run);

		err = attr_load_runs_range(ni, ATTR_EA, NULL, 0, &run, 0, size);
		if (!err)
			err = ntfs_read_run_nb(sbi, &run, 0, ea_p, size, NULL);
		run_close(&run);

		if (err)
			goto out;
	} else {
		void *p = resident_data_ex(attr_ea, size);

		if (!p) {
			err = -EINVAL;
			goto out;
		}
		memcpy(ea_p, p, size);
	}

	memset(Add2Ptr(ea_p, size), 0, add_bytes);
	*ea = ea_p;
	return 0;

out:
	kfree(ea_p);
	*ea = NULL;
	return err;
}
