static int pty_write(struct tty_struct *tty, const unsigned char *buf, int c)
{
	struct tty_struct *to = tty->link;

	if (tty->flow.stopped || !c)
		return 0;

	return tty_insert_flip_string_and_push_buffer(to->port, buf, c);
}
