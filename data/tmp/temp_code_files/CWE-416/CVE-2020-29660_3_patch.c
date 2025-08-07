static int tiocgsid(struct tty_struct *tty, struct tty_struct *real_tty, pid_t __user *p)
{
	unsigned long flags;
	pid_t sid;

	/*
	 * (tty == real_tty) is a cheap way of
	 * testing if the tty is NOT a master pty.
	*/
	if (tty == real_tty && current->signal->tty != real_tty)
		return -ENOTTY;

	spin_lock_irqsave(&real_tty->ctrl_lock, flags);
	if (!real_tty->session)
		goto err;
	sid = pid_vnr(real_tty->session);
	spin_unlock_irqrestore(&real_tty->ctrl_lock, flags);

	return put_user(sid, p);

err:
	spin_unlock_irqrestore(&real_tty->ctrl_lock, flags);
	return -ENOTTY;
}