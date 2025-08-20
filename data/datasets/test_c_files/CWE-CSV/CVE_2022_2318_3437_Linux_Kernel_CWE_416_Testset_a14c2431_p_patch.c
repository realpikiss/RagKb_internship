void rose_start_t3timer(struct sock *sk)
{
	struct rose_sock *rose = rose_sk(sk);

	sk_stop_timer(sk, &rose->timer);

	rose->timer.function = rose_timer_expiry;
	rose->timer.expires  = jiffies + rose->t3;

	sk_reset_timer(sk, &rose->timer, rose->timer.expires);
}
