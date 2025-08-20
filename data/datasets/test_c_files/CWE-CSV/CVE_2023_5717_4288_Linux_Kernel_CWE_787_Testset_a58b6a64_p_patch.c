static void perf_group_detach(struct perf_event *event)
{
	struct perf_event *leader = event->group_leader;
	struct perf_event *sibling, *tmp;
	struct perf_event_context *ctx = event->ctx;

	lockdep_assert_held(&ctx->lock);

	/*
	 * We can have double detach due to exit/hot-unplug + close.
	 */
	if (!(event->attach_state & PERF_ATTACH_GROUP))
		return;

	event->attach_state &= ~PERF_ATTACH_GROUP;

	perf_put_aux_event(event);

	/*
	 * If this is a sibling, remove it from its group.
	 */
	if (leader != event) {
		list_del_init(&event->sibling_list);
		event->group_leader->nr_siblings--;
		event->group_leader->group_generation++;
		goto out;
	}

	/*
	 * If this was a group event with sibling events then
	 * upgrade the siblings to singleton events by adding them
	 * to whatever list we are on.
	 */
	list_for_each_entry_safe(sibling, tmp, &event->sibling_list, sibling_list) {

		if (sibling->event_caps & PERF_EV_CAP_SIBLING)
			perf_remove_sibling_event(sibling);

		sibling->group_leader = sibling;
		list_del_init(&sibling->sibling_list);

		/* Inherit group flags from the previous leader */
		sibling->group_caps = event->group_caps;

		if (sibling->attach_state & PERF_ATTACH_CONTEXT) {
			add_event_to_groups(sibling, event->ctx);

			if (sibling->state == PERF_EVENT_STATE_ACTIVE)
				list_add_tail(&sibling->active_list, get_event_list(sibling));
		}

		WARN_ON_ONCE(sibling->ctx != event->ctx);
	}

out:
	for_each_sibling_event(tmp, leader)
		perf_event__header_size(tmp);

	perf_event__header_size(leader);
}
