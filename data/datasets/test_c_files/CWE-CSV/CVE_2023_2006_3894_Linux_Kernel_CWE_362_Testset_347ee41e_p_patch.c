int rxrpc_connect_call(struct rxrpc_sock *rx,
		       struct rxrpc_call *call,
		       struct rxrpc_conn_parameters *cp,
		       struct sockaddr_rxrpc *srx,
		       gfp_t gfp)
{
	struct rxrpc_bundle *bundle;
	struct rxrpc_net *rxnet = cp->local->rxnet;
	int ret = 0;

	_enter("{%d,%lx},", call->debug_id, call->user_call_ID);

	rxrpc_discard_expired_client_conns(&rxnet->client_conn_reaper);

	bundle = rxrpc_prep_call(rx, call, cp, srx, gfp);
	if (IS_ERR(bundle)) {
		ret = PTR_ERR(bundle);
		goto out;
	}

	if (call->state == RXRPC_CALL_CLIENT_AWAIT_CONN) {
		ret = rxrpc_wait_for_channel(bundle, call, gfp);
		if (ret < 0)
			goto wait_failed;
	}

granted_channel:
	/* Paired with the write barrier in rxrpc_activate_one_channel(). */
	smp_rmb();

out_put_bundle:
	rxrpc_deactivate_bundle(bundle);
	rxrpc_put_bundle(bundle);
out:
	_leave(" = %d", ret);
	return ret;

wait_failed:
	spin_lock(&bundle->channel_lock);
	list_del_init(&call->chan_wait_link);
	spin_unlock(&bundle->channel_lock);

	if (call->state != RXRPC_CALL_CLIENT_AWAIT_CONN) {
		ret = 0;
		goto granted_channel;
	}

	trace_rxrpc_client(call->conn, ret, rxrpc_client_chan_wait_failed);
	rxrpc_set_call_completion(call, RXRPC_CALL_LOCAL_ERROR, 0, ret);
	rxrpc_disconnect_client_call(bundle, call);
	goto out_put_bundle;
}
