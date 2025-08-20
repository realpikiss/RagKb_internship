int gru_check_context_placement(struct gru_thread_state *gts)
{
	struct gru_state *gru;
	int ret = 0;

	/*
	 * If the current task is the context owner, verify that the
	 * context is correctly placed. This test is skipped for non-owner
	 * references. Pthread apps use non-owner references to the CBRs.
	 */
	gru = gts->ts_gru;
	/*
	 * If gru or gts->ts_tgid_owner isn't initialized properly, return
	 * success to indicate that the caller does not need to unload the
	 * gru context.The caller is responsible for their inspection and
	 * reinitialization if needed.
	 */
	if (!gru || gts->ts_tgid_owner != current->tgid)
		return ret;

	if (!gru_check_chiplet_assignment(gru, gts)) {
		STAT(check_context_unload);
		ret = -EINVAL;
	} else if (gru_retarget_intr(gts)) {
		STAT(check_context_retarget_intr);
	}

	return ret;
}
