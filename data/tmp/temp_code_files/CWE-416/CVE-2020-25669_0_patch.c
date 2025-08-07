static void sunkbd_enable(struct sunkbd *sunkbd, bool enable)
{
	serio_pause_rx(sunkbd->serio);
	sunkbd->enabled = enable;
	serio_continue_rx(sunkbd->serio);

	if (!enable) {
		wake_up_interruptible(&sunkbd->wait);
		cancel_work_sync(&sunkbd->tq);
	}
}