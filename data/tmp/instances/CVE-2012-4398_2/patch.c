static int uvesafb_helper_start(void)
{
	char *envp[] = {
		"HOME=/",
		"PATH=/sbin:/bin",
		NULL,
	};

	char *argv[] = {
		v86d_path,
		NULL,
	};

	return call_usermodehelper(v86d_path, argv, envp, UMH_WAIT_PROC);
}