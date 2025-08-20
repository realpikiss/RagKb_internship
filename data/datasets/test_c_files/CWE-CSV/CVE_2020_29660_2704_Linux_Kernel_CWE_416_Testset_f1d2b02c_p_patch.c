static int tiocspgrp(struct tty_struct *tty, struct tty_struct *real_tty, pid_t __user *p)
{
	struct pid *pgrp;
	pid_t pgrp_nr;
	int retval = tty_check_change(real_tty);

	if (retval == -EIO)
		return -ENOTTY;
	if (retval)
		return retval;

	if (get_user(pgrp_nr, p))
		return -EFAULT;
	if (pgrp_nr < 0)
		return -EINVAL;

	spin_lock_irq(&real_tty->ctrl_lock);
	if (!current->signal->tty ||
	    (current->signal->tty != real_tty) ||
	    (real_tty->session != task_session(current))) {
		retval = -ENOTTY;
		goto out_unlock_ctrl;
	}
	rcu_read_lock();
	pgrp = find_vpid(pgrp_nr);
	retval = -ESRCH;
	if (!pgrp)
		goto out_unlock;
	retval = -EPERM;
	if (session_of_pgrp(pgrp) != task_session(current))
		goto out_unlock;
	retval = 0;
	put_pid(real_tty->pgrp);
	real_tty->pgrp = get_pid(pgrp);
out_unlock:
	rcu_read_unlock();
out_unlock_ctrl:
	spin_unlock_irq(&real_tty->ctrl_lock);
	return retval;
}
