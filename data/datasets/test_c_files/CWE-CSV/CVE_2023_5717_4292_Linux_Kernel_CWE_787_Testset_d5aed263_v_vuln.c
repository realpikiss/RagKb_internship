static void perf_group_attach(struct perf_event *event)
{
	struct perf_event *group_leader = event->group_leader, *pos;

	lockdep_assert_held(&event->ctx->lock);

	/*
	 * We can have double attach due to group movement (move_group) in
	 * perf_event_open().
	 */
	if (event->attach_state & PERF_ATTACH_GROUP)
		return;

	event->attach_state |= PERF_ATTACH_GROUP;

	if (group_leader == event)
		return;

	WARN_ON_ONCE(group_leader->ctx != event->ctx);

	group_leader->group_caps &= event->event_caps;

	list_add_tail(&event->sibling_list, &group_leader->sibling_list);
	group_leader->nr_siblings++;

	perf_event__header_size(group_leader);

	for_each_sibling_event(pos, group_leader)
		perf_event__header_size(pos);
}
