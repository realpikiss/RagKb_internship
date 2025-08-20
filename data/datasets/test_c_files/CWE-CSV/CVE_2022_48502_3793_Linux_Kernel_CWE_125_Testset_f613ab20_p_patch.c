int indx_read(struct ntfs_index *indx, struct ntfs_inode *ni, CLST vbn,
	      struct indx_node **node)
{
	int err;
	struct INDEX_BUFFER *ib;
	struct runs_tree *run = &indx->alloc_run;
	struct rw_semaphore *lock = &indx->run_lock;
	u64 vbo = (u64)vbn << indx->vbn2vbo_bits;
	u32 bytes = 1u << indx->index_bits;
	struct indx_node *in = *node;
	const struct INDEX_NAMES *name;

	if (!in) {
		in = kzalloc(sizeof(struct indx_node), GFP_NOFS);
		if (!in)
			return -ENOMEM;
	} else {
		nb_put(&in->nb);
	}

	ib = in->index;
	if (!ib) {
		ib = kmalloc(bytes, GFP_NOFS);
		if (!ib) {
			err = -ENOMEM;
			goto out;
		}
	}

	down_read(lock);
	err = ntfs_read_bh(ni->mi.sbi, run, vbo, &ib->rhdr, bytes, &in->nb);
	up_read(lock);
	if (!err)
		goto ok;

	if (err == -E_NTFS_FIXUP)
		goto ok;

	if (err != -ENOENT)
		goto out;

	name = &s_index_names[indx->type];
	down_write(lock);
	err = attr_load_runs_range(ni, ATTR_ALLOC, name->name, name->name_len,
				   run, vbo, vbo + bytes);
	up_write(lock);
	if (err)
		goto out;

	down_read(lock);
	err = ntfs_read_bh(ni->mi.sbi, run, vbo, &ib->rhdr, bytes, &in->nb);
	up_read(lock);
	if (err == -E_NTFS_FIXUP)
		goto ok;

	if (err)
		goto out;

ok:
	if (!index_buf_check(ib, bytes, &vbn)) {
		ntfs_inode_err(&ni->vfs_inode, "directory corrupted");
		ntfs_set_state(ni->mi.sbi, NTFS_DIRTY_ERROR);
		err = -EINVAL;
		goto out;
	}

	if (err == -E_NTFS_FIXUP) {
		ntfs_write_bh(ni->mi.sbi, &ib->rhdr, &in->nb, 0);
		err = 0;
	}

	/* check for index header length */
	if (offsetof(struct INDEX_BUFFER, ihdr) + ib->ihdr.used > bytes) {
		err = -EINVAL;
		goto out;
	}

	in->index = ib;
	*node = in;

out:
	if (ib != in->index)
		kfree(ib);

	if (*node != in) {
		nb_put(&in->nb);
		kfree(in);
	}

	return err;
}
