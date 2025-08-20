static void __mctp_key_remove(struct mctp_sk_key *key, struct net *net,
			      unsigned long flags, unsigned long reason)
__releases(&key->lock)
__must_hold(&net->mctp.keys_lock)
{
	struct sk_buff *skb;

	trace_mctp_key_release(key, reason);
	skb = key->reasm_head;
	key->reasm_head = NULL;
	key->reasm_dead = true;
	key->valid = false;
	mctp_dev_release_key(key->dev, key);
	spin_unlock_irqrestore(&key->lock, flags);

	if (!hlist_unhashed(&key->hlist)) {
		hlist_del_init(&key->hlist);
		hlist_del_init(&key->sklist);
		/* unref for the lists */
		mctp_key_unref(key);
	}

	kfree_skb(skb);
}
