static void xfrm_policy_kill(struct xfrm_policy *policy)
{
	write_lock_bh(&policy->lock);
	policy->walk.dead = 1;
	write_unlock_bh(&policy->lock);

	atomic_inc(&policy->genid);

	if (del_timer(&policy->polq.hold_timer))
		xfrm_pol_put(policy);
	skb_queue_purge(&policy->polq.hold_queue);

	if (del_timer(&policy->timer))
		xfrm_pol_put(policy);

	xfrm_pol_put(policy);
}