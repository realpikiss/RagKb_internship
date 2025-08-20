void __fscache_invalidate(struct fscache_cookie *cookie,
			  const void *aux_data, loff_t new_size,
			  unsigned int flags)
{
	bool is_caching;

	_enter("c=%x", cookie->debug_id);

	fscache_stat(&fscache_n_invalidates);

	if (WARN(test_bit(FSCACHE_COOKIE_RELINQUISHED, &cookie->flags),
		 "Trying to invalidate relinquished cookie\n"))
		return;

	if ((flags & FSCACHE_INVAL_DIO_WRITE) &&
	    test_and_set_bit(FSCACHE_COOKIE_DISABLED, &cookie->flags))
		return;

	spin_lock(&cookie->lock);
	set_bit(FSCACHE_COOKIE_NO_DATA_TO_READ, &cookie->flags);
	fscache_update_aux(cookie, aux_data, &new_size);
	cookie->inval_counter++;
	trace_fscache_invalidate(cookie, new_size);

	switch (cookie->state) {
	case FSCACHE_COOKIE_STATE_INVALIDATING: /* is_still_valid will catch it */
	default:
		spin_unlock(&cookie->lock);
		_leave(" [no %u]", cookie->state);
		return;

	case FSCACHE_COOKIE_STATE_LOOKING_UP:
		__fscache_begin_cookie_access(cookie, fscache_access_invalidate_cookie);
		set_bit(FSCACHE_COOKIE_DO_INVALIDATE, &cookie->flags);
		fallthrough;
	case FSCACHE_COOKIE_STATE_CREATING:
		spin_unlock(&cookie->lock);
		_leave(" [look %x]", cookie->inval_counter);
		return;

	case FSCACHE_COOKIE_STATE_ACTIVE:
		is_caching = fscache_begin_cookie_access(
			cookie, fscache_access_invalidate_cookie);
		if (is_caching)
			__fscache_set_cookie_state(cookie, FSCACHE_COOKIE_STATE_INVALIDATING);
		spin_unlock(&cookie->lock);
		wake_up_cookie_state(cookie);

		if (is_caching)
			fscache_queue_cookie(cookie, fscache_cookie_get_inval_work);
		_leave(" [inv]");
		return;
	}
}
