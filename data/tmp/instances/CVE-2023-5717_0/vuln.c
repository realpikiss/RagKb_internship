static int inherit_group(struct perf_event *parent_event,
	      struct task_struct *parent,
	      struct perf_event_context *parent_ctx,
	      struct task_struct *child,
	      struct perf_event_context *child_ctx)
{
	struct perf_event *leader;
	struct perf_event *sub;
	struct perf_event *child_ctr;

	leader = inherit_event(parent_event, parent, parent_ctx,
				 child, NULL, child_ctx);
	if (IS_ERR(leader))
		return PTR_ERR(leader);
	/*
	 * @leader can be NULL here because of is_orphaned_event(). In this
	 * case inherit_event() will create individual events, similar to what
	 * perf_group_detach() would do anyway.
	 */
	for_each_sibling_event(sub, parent_event) {
		child_ctr = inherit_event(sub, parent, parent_ctx,
					    child, leader, child_ctx);
		if (IS_ERR(child_ctr))
			return PTR_ERR(child_ctr);

		if (sub->aux_event == parent_event && child_ctr &&
		    !perf_get_aux_event(child_ctr, leader))
			return -EINVAL;
	}
	return 0;
}