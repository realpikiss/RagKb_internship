static int ptrace_attach(struct task_struct *task, long request,
			 unsigned long addr,
			 unsigned long flags)
{
	bool seize = (request == PTRACE_SEIZE);
	int retval;

	retval = -EIO;
	if (seize) {
		if (addr != 0)
			goto out;
		if (flags & ~(unsigned long)PTRACE_O_MASK)
			goto out;
		flags = PT_PTRACED | PT_SEIZED | (flags << PT_OPT_FLAG_SHIFT);
	} else {
		flags = PT_PTRACED;
	}

	audit_ptrace(task);

	retval = -EPERM;
	if (unlikely(task->flags & PF_KTHREAD))
		goto out;
	if (same_thread_group(task, current))
		goto out;

	/*
	 * Protect exec's credential calculations against our interference;
	 * SUID, SGID and LSM creds get determined differently
	 * under ptrace.
	 */
	retval = -ERESTARTNOINTR;
	if (mutex_lock_interruptible(&task->signal->cred_guard_mutex))
		goto out;

	task_lock(task);
	retval = __ptrace_may_access(task, PTRACE_MODE_ATTACH);
	task_unlock(task);
	if (retval)
		goto unlock_creds;

	write_lock_irq(&tasklist_lock);
	retval = -EPERM;
	if (unlikely(task->exit_state))
		goto unlock_tasklist;
	if (task->ptrace)
		goto unlock_tasklist;

	if (seize)
		flags |= PT_SEIZED;
	rcu_read_lock();
	if (ns_capable(__task_cred(task)->user_ns, CAP_SYS_PTRACE))
		flags |= PT_PTRACE_CAP;
	rcu_read_unlock();
	task->ptrace = flags;

	__ptrace_link(task, current);

	/* SEIZE doesn't trap tracee on attach */
	if (!seize)
		send_sig_info(SIGSTOP, SEND_SIG_FORCED, task);

	spin_lock(&task->sighand->siglock);

	/*
	 * If the task is already STOPPED, set JOBCTL_TRAP_STOP and
	 * TRAPPING, and kick it so that it transits to TRACED.  TRAPPING
	 * will be cleared if the child completes the transition or any
	 * event which clears the group stop states happens.  We'll wait
	 * for the transition to complete before returning from this
	 * function.
	 *
	 * This hides STOPPED -> RUNNING -> TRACED transition from the
	 * attaching thread but a different thread in the same group can
	 * still observe the transient RUNNING state.  IOW, if another
	 * thread's WNOHANG wait(2) on the stopped tracee races against
	 * ATTACH, the wait(2) may fail due to the transient RUNNING.
	 *
	 * The following task_is_stopped() test is safe as both transitions
	 * in and out of STOPPED are protected by siglock.
	 */
	if (task_is_stopped(task) &&
	    task_set_jobctl_pending(task, JOBCTL_TRAP_STOP | JOBCTL_TRAPPING))
		signal_wake_up_state(task, __TASK_STOPPED);

	spin_unlock(&task->sighand->siglock);

	retval = 0;
unlock_tasklist:
	write_unlock_irq(&tasklist_lock);
unlock_creds:
	mutex_unlock(&task->signal->cred_guard_mutex);
out:
	if (!retval) {
		wait_on_bit(&task->jobctl, JOBCTL_TRAPPING_BIT,
			    ptrace_trapping_sleep_fn, TASK_UNINTERRUPTIBLE);
		proc_ptrace_connector(task, PTRACE_ATTACH);
	}

	return retval;
}
