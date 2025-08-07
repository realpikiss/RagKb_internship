static __be16 sctp_process_asconf_param(struct sctp_association *asoc,
					struct sctp_chunk *asconf,
					struct sctp_addip_param *asconf_param)
{
	union sctp_addr_param *addr_param;
	struct sctp_transport *peer;
	union sctp_addr	addr;
	struct sctp_af *af;

	addr_param = (void *)asconf_param + sizeof(*asconf_param);

	if (asconf_param->param_hdr.type != SCTP_PARAM_ADD_IP &&
	    asconf_param->param_hdr.type != SCTP_PARAM_DEL_IP &&
	    asconf_param->param_hdr.type != SCTP_PARAM_SET_PRIMARY)
		return SCTP_ERROR_UNKNOWN_PARAM;

	switch (addr_param->p.type) {
	case SCTP_PARAM_IPV6_ADDRESS:
		if (!asoc->peer.ipv6_address)
			return SCTP_ERROR_DNS_FAILED;
		break;
	case SCTP_PARAM_IPV4_ADDRESS:
		if (!asoc->peer.ipv4_address)
			return SCTP_ERROR_DNS_FAILED;
		break;
	default:
		return SCTP_ERROR_DNS_FAILED;
	}

	af = sctp_get_af_specific(param_type2af(addr_param->p.type));
	if (unlikely(!af))
		return SCTP_ERROR_DNS_FAILED;

	if (!af->from_addr_param(&addr, addr_param, htons(asoc->peer.port), 0))
		return SCTP_ERROR_DNS_FAILED;

	/* ADDIP 4.2.1  This parameter MUST NOT contain a broadcast
	 * or multicast address.
	 * (note: wildcard is permitted and requires special handling so
	 *  make sure we check for that)
	 */
	if (!af->is_any(&addr) && !af->addr_valid(&addr, NULL, asconf->skb))
		return SCTP_ERROR_DNS_FAILED;

	switch (asconf_param->param_hdr.type) {
	case SCTP_PARAM_ADD_IP:
		/* Section 4.2.1:
		 * If the address 0.0.0.0 or ::0 is provided, the source
		 * address of the packet MUST be added.
		 */
		if (af->is_any(&addr))
			memcpy(&addr, &asconf->source, sizeof(addr));

		if (security_sctp_bind_connect(asoc->ep->base.sk,
					       SCTP_PARAM_ADD_IP,
					       (struct sockaddr *)&addr,
					       af->sockaddr_len))
			return SCTP_ERROR_REQ_REFUSED;

		/* ADDIP 4.3 D9) If an endpoint receives an ADD IP address
		 * request and does not have the local resources to add this
		 * new address to the association, it MUST return an Error
		 * Cause TLV set to the new error code 'Operation Refused
		 * Due to Resource Shortage'.
		 */

		peer = sctp_assoc_add_peer(asoc, &addr, GFP_ATOMIC, SCTP_UNCONFIRMED);
		if (!peer)
			return SCTP_ERROR_RSRC_LOW;

		/* Start the heartbeat timer. */
		sctp_transport_reset_hb_timer(peer);
		asoc->new_transport = peer;
		break;
	case SCTP_PARAM_DEL_IP:
		/* ADDIP 4.3 D7) If a request is received to delete the
		 * last remaining IP address of a peer endpoint, the receiver
		 * MUST send an Error Cause TLV with the error cause set to the
		 * new error code 'Request to Delete Last Remaining IP Address'.
		 */
		if (asoc->peer.transport_count == 1)
			return SCTP_ERROR_DEL_LAST_IP;

		/* ADDIP 4.3 D8) If a request is received to delete an IP
		 * address which is also the source address of the IP packet
		 * which contained the ASCONF chunk, the receiver MUST reject
		 * this request. To reject the request the receiver MUST send
		 * an Error Cause TLV set to the new error code 'Request to
		 * Delete Source IP Address'
		 */
		if (sctp_cmp_addr_exact(&asconf->source, &addr))
			return SCTP_ERROR_DEL_SRC_IP;

		/* Section 4.2.2
		 * If the address 0.0.0.0 or ::0 is provided, all
		 * addresses of the peer except	the source address of the
		 * packet MUST be deleted.
		 */
		if (af->is_any(&addr)) {
			sctp_assoc_set_primary(asoc, asconf->transport);
			sctp_assoc_del_nonprimary_peers(asoc,
							asconf->transport);
			return SCTP_ERROR_NO_ERROR;
		}

		/* If the address is not part of the association, the
		 * ASCONF-ACK with Error Cause Indication Parameter
		 * which including cause of Unresolvable Address should
		 * be sent.
		 */
		peer = sctp_assoc_lookup_paddr(asoc, &addr);
		if (!peer)
			return SCTP_ERROR_DNS_FAILED;

		sctp_assoc_rm_peer(asoc, peer);
		break;
	case SCTP_PARAM_SET_PRIMARY:
		/* ADDIP Section 4.2.4
		 * If the address 0.0.0.0 or ::0 is provided, the receiver
		 * MAY mark the source address of the packet as its
		 * primary.
		 */
		if (af->is_any(&addr))
			memcpy(&addr, sctp_source(asconf), sizeof(addr));

		if (security_sctp_bind_connect(asoc->ep->base.sk,
					       SCTP_PARAM_SET_PRIMARY,
					       (struct sockaddr *)&addr,
					       af->sockaddr_len))
			return SCTP_ERROR_REQ_REFUSED;

		peer = sctp_assoc_lookup_paddr(asoc, &addr);
		if (!peer)
			return SCTP_ERROR_DNS_FAILED;

		sctp_assoc_set_primary(asoc, peer);
		break;
	}

	return SCTP_ERROR_NO_ERROR;
}