void rose_start_idletimer(struct sock *sk)
{
	struct rose_sock *rose = rose_sk(sk);

	sk_stop_timer(sk, &rose->idletimer);

	if (rose->idle > 0) {
		rose->idletimer.function = rose_idletimer_expiry;
		rose->idletimer.expires  = jiffies + rose->idle;

		sk_reset_timer(sk, &rose->idletimer, rose->idletimer.expires);
	}
}
