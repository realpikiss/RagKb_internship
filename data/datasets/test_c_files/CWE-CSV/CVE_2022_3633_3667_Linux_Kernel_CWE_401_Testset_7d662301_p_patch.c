static void j1939_session_destroy(struct j1939_session *session)
{
	struct sk_buff *skb;

	if (session->transmission) {
		if (session->err)
			j1939_sk_errqueue(session, J1939_ERRQUEUE_TX_ABORT);
		else
			j1939_sk_errqueue(session, J1939_ERRQUEUE_TX_ACK);
	} else if (session->err) {
			j1939_sk_errqueue(session, J1939_ERRQUEUE_RX_ABORT);
	}

	netdev_dbg(session->priv->ndev, "%s: 0x%p\n", __func__, session);

	WARN_ON_ONCE(!list_empty(&session->sk_session_queue_entry));
	WARN_ON_ONCE(!list_empty(&session->active_session_list_entry));

	while ((skb = skb_dequeue(&session->skb_queue)) != NULL) {
		/* drop ref taken in j1939_session_skb_queue() */
		skb_unref(skb);
		kfree_skb(skb);
	}
	__j1939_session_drop(session);
	j1939_priv_put(session->priv);
	kfree(session);
}
