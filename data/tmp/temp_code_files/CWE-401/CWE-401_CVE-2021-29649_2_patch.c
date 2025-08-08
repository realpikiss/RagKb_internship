static void __exit fini_umd(void)
{
	struct pid *tgid;

	bpf_preload_ops = NULL;

	/* kill UMD in case it's still there due to earlier error */
	tgid = umd_ops.info.tgid;
	if (tgid) {
		kill_pid(tgid, SIGKILL, 1);

		wait_event(tgid->wait_pidfd, thread_group_exited(tgid));
		umd_cleanup_helper(&umd_ops.info);
	}
	umd_unload_blob(&umd_ops.info);
}