static void mntput_no_expire(struct mount *mnt)
{
	LIST_HEAD(list);

	rcu_read_lock();
	if (likely(READ_ONCE(mnt->mnt_ns))) {
		/*
		 * Since we don't do lock_mount_hash() here,
		 * ->mnt_ns can change under us.  However, if it's
		 * non-NULL, then there's a reference that won't
		 * be dropped until after an RCU delay done after
		 * turning ->mnt_ns NULL.  So if we observe it
		 * non-NULL under rcu_read_lock(), the reference
		 * we are dropping is not the final one.
		 */
		mnt_add_count(mnt, -1);
		rcu_read_unlock();
		return;
	}
	lock_mount_hash();
	/*
	 * make sure that if __legitimize_mnt() has not seen us grab
	 * mount_lock, we'll see their refcount increment here.
	 */
	smp_mb();
	mnt_add_count(mnt, -1);
	if (mnt_get_count(mnt)) {
		rcu_read_unlock();
		unlock_mount_hash();
		return;
	}
	if (unlikely(mnt->mnt.mnt_flags & MNT_DOOMED)) {
		rcu_read_unlock();
		unlock_mount_hash();
		return;
	}
	mnt->mnt.mnt_flags |= MNT_DOOMED;
	rcu_read_unlock();

	list_del(&mnt->mnt_instance);

	if (unlikely(!list_empty(&mnt->mnt_mounts))) {
		struct mount *p, *tmp;
		list_for_each_entry_safe(p, tmp, &mnt->mnt_mounts,  mnt_child) {
			__put_mountpoint(unhash_mnt(p), &list);
		}
	}
	unlock_mount_hash();
	shrink_dentry_list(&list);

	if (likely(!(mnt->mnt.mnt_flags & MNT_INTERNAL))) {
		struct task_struct *task = current;
		if (likely(!(task->flags & PF_KTHREAD))) {
			init_task_work(&mnt->mnt_rcu, __cleanup_mnt);
			if (!task_work_add(task, &mnt->mnt_rcu, true))
				return;
		}
		if (llist_add(&mnt->mnt_llist, &delayed_mntput_list))
			schedule_delayed_work(&delayed_mntput_work, 1);
		return;
	}
	cleanup_mnt(mnt);
}
