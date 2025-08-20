static void j1939_session_destroy(struct j1939_session *session)
{
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

	skb_queue_purge(&session->skb_queue);
	__j1939_session_drop(session);
	j1939_priv_put(session->priv);
	kfree(session);
}
