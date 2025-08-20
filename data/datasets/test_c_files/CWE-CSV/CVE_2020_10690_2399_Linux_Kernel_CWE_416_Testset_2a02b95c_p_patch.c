int ptp_clock_unregister(struct ptp_clock *ptp)
{
	ptp->defunct = 1;
	wake_up_interruptible(&ptp->tsev_wq);

	if (ptp->kworker) {
		kthread_cancel_delayed_work_sync(&ptp->aux_work);
		kthread_destroy_worker(ptp->kworker);
	}

	/* Release the clock's resources. */
	if (ptp->pps_source)
		pps_unregister_source(ptp->pps_source);

	ptp_cleanup_pin_groups(ptp);

	posix_clock_unregister(&ptp->clock);
	return 0;
}
