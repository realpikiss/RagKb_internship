static int pty_write(struct tty_struct *tty, const unsigned char *buf, int c)
{
	struct tty_struct *to = tty->link;
	unsigned long flags;

	if (tty->flow.stopped)
		return 0;

	if (c > 0) {
		spin_lock_irqsave(&to->port->lock, flags);
		/* Stuff the data into the input queue of the other end */
		c = tty_insert_flip_string(to->port, buf, c);
		spin_unlock_irqrestore(&to->port->lock, flags);
		/* And shovel */
		if (c)
			tty_flip_buffer_push(to->port);
	}
	return c;
}
