static void slcan_write_wakeup(struct tty_struct *tty)
{
	struct slcan *sl;

	rcu_read_lock();
	sl = rcu_dereference(tty->disc_data);
	if (!sl)
		goto out;

	schedule_work(&sl->tx_work);
out:
	rcu_read_unlock();
}
