static void mctp_serial_close(struct tty_struct *tty)
{
	struct mctp_serial *dev = tty->disc_data;
	int idx = dev->idx;

	unregister_netdev(dev->netdev);
	ida_free(&mctp_serial_ida, idx);
}