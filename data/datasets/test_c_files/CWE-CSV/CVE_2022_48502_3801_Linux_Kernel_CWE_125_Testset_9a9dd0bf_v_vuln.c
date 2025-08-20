static ssize_t ntfs_list_ea(struct ntfs_inode *ni, char *buffer,
			    size_t bytes_per_buffer)
{
	const struct EA_INFO *info;
	struct EA_FULL *ea_all = NULL;
	const struct EA_FULL *ea;
	u32 off, size;
	int err;
	size_t ret;

	err = ntfs_read_ea(ni, &ea_all, 0, &info);
	if (err)
		return err;

	if (!info || !ea_all)
		return 0;

	size = le32_to_cpu(info->size);

	/* Enumerate all xattrs. */
	for (ret = 0, off = 0; off < size; off += unpacked_ea_size(ea)) {
		ea = Add2Ptr(ea_all, off);

		if (buffer) {
			if (ret + ea->name_len + 1 > bytes_per_buffer) {
				err = -ERANGE;
				goto out;
			}

			memcpy(buffer + ret, ea->name, ea->name_len);
			buffer[ret + ea->name_len] = 0;
		}

		ret += ea->name_len + 1;
	}

out:
	kfree(ea_all);
	return err ? err : ret;
}
