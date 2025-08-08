static void perf_mmap_close(struct vm_area_struct *vma)
{
	struct perf_event *event = vma->vm_file->private_data;
	struct perf_buffer *rb = ring_buffer_get(event);
	struct user_struct *mmap_user = rb->mmap_user;
	int mmap_locked = rb->mmap_locked;
	unsigned long size = perf_data_size(rb);
	bool detach_rest = false;

	if (event->pmu->event_unmapped)
		event->pmu->event_unmapped(event, vma->vm_mm);

	/*
	 * rb->aux_mmap_count will always drop before rb->mmap_count and
	 * event->mmap_count, so it is ok to use event->mmap_mutex to
	 * serialize with perf_mmap here.
	 */
	if (rb_has_aux(rb) && vma->vm_pgoff == rb->aux_pgoff &&
	    atomic_dec_and_mutex_lock(&rb->aux_mmap_count, &event->mmap_mutex)) {
		/*
		 * Stop all AUX events that are writing to this buffer,
		 * so that we can free its AUX pages and corresponding PMU
		 * data. Note that after rb::aux_mmap_count dropped to zero,
		 * they won't start any more (see perf_aux_output_begin()).
		 */
		perf_pmu_output_stop(event);

		/* now it's safe to free the pages */
		atomic_long_sub(rb->aux_nr_pages - rb->aux_mmap_locked, &mmap_user->locked_vm);
		atomic64_sub(rb->aux_mmap_locked, &vma->vm_mm->pinned_vm);

		/* this has to be the last one */
		rb_free_aux(rb);
		WARN_ON_ONCE(refcount_read(&rb->aux_refcount));

		mutex_unlock(&event->mmap_mutex);
	}

	if (atomic_dec_and_test(&rb->mmap_count))
		detach_rest = true;

	if (!atomic_dec_and_mutex_lock(&event->mmap_count, &event->mmap_mutex))
		goto out_put;

	ring_buffer_attach(event, NULL);
	mutex_unlock(&event->mmap_mutex);

	/* If there's still other mmap()s of this buffer, we're done. */
	if (!detach_rest)
		goto out_put;

	/*
	 * No other mmap()s, detach from all other events that might redirect
	 * into the now unreachable buffer. Somewhat complicated by the
	 * fact that rb::event_lock otherwise nests inside mmap_mutex.
	 */
again:
	rcu_read_lock();
	list_for_each_entry_rcu(event, &rb->event_list, rb_entry) {
		if (!atomic_long_inc_not_zero(&event->refcount)) {
			/*
			 * This event is en-route to free_event() which will
			 * detach it and remove it from the list.
			 */
			continue;
		}
		rcu_read_unlock();

		mutex_lock(&event->mmap_mutex);
		/*
		 * Check we didn't race with perf_event_set_output() which can
		 * swizzle the rb from under us while we were waiting to
		 * acquire mmap_mutex.
		 *
		 * If we find a different rb; ignore this event, a next
		 * iteration will no longer find it on the list. We have to
		 * still restart the iteration to make sure we're not now
		 * iterating the wrong list.
		 */
		if (event->rb == rb)
			ring_buffer_attach(event, NULL);

		mutex_unlock(&event->mmap_mutex);
		put_event(event);

		/*
		 * Restart the iteration; either we're on the wrong list or
		 * destroyed its integrity by doing a deletion.
		 */
		goto again;
	}
	rcu_read_unlock();

	/*
	 * It could be there's still a few 0-ref events on the list; they'll
	 * get cleaned up by free_event() -- they'll also still have their
	 * ref on the rb and will free it whenever they are done with it.
	 *
	 * Aside from that, this buffer is 'fully' detached and unmapped,
	 * undo the VM accounting.
	 */

	atomic_long_sub((size >> PAGE_SHIFT) + 1 - mmap_locked,
			&mmap_user->locked_vm);
	atomic64_sub(mmap_locked, &vma->vm_mm->pinned_vm);
	free_uid(mmap_user);

out_put:
	ring_buffer_put(rb); /* could be last */
}