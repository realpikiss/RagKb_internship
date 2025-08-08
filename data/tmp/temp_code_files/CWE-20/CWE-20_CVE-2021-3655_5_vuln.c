static void sctp_asconf_param_success(struct sctp_association *asoc,
				      struct sctp_addip_param *asconf_param)
{
	struct sctp_bind_addr *bp = &asoc->base.bind_addr;
	union sctp_addr_param *addr_param;
	struct sctp_sockaddr_entry *saddr;
	struct sctp_transport *transport;
	union sctp_addr	addr;
	struct sctp_af *af;

	addr_param = (void *)asconf_param + sizeof(*asconf_param);

	/* We have checked the packet before, so we do not check again.	*/
	af = sctp_get_af_specific(param_type2af(addr_param->p.type));
	af->from_addr_param(&addr, addr_param, htons(bp->port), 0);

	switch (asconf_param->param_hdr.type) {
	case SCTP_PARAM_ADD_IP:
		/* This is always done in BH context with a socket lock
		 * held, so the list can not change.
		 */
		local_bh_disable();
		list_for_each_entry(saddr, &bp->address_list, list) {
			if (sctp_cmp_addr_exact(&saddr->a, &addr))
				saddr->state = SCTP_ADDR_SRC;
		}
		local_bh_enable();
		list_for_each_entry(transport, &asoc->peer.transport_addr_list,
				transports) {
			sctp_transport_dst_release(transport);
		}
		break;
	case SCTP_PARAM_DEL_IP:
		local_bh_disable();
		sctp_del_bind_addr(bp, &addr);
		if (asoc->asconf_addr_del_pending != NULL &&
		    sctp_cmp_addr_exact(asoc->asconf_addr_del_pending, &addr)) {
			kfree(asoc->asconf_addr_del_pending);
			asoc->asconf_addr_del_pending = NULL;
		}
		local_bh_enable();
		list_for_each_entry(transport, &asoc->peer.transport_addr_list,
				transports) {
			sctp_transport_dst_release(transport);
		}
		break;
	default:
		break;
	}
}