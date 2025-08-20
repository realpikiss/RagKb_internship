void rose_start_hbtimer(struct sock *sk)
{
	struct rose_sock *rose = rose_sk(sk);

	sk_stop_timer(sk, &rose->timer);

	rose->timer.function = rose_timer_expiry;
	rose->timer.expires  = jiffies + rose->hb;

	sk_reset_timer(sk, &rose->timer, rose->timer.expires);
}
