static struct sctp_association *__sctp_rcv_asconf_lookup(
					struct net *net,
					struct sctp_chunkhdr *ch,
					const union sctp_addr *laddr,
					__be16 peer_port,
					struct sctp_transport **transportp)
{
	struct sctp_addip_chunk *asconf = (struct sctp_addip_chunk *)ch;
	struct sctp_af *af;
	union sctp_addr_param *param;
	union sctp_addr paddr;

	/* Skip over the ADDIP header and find the Address parameter */
	param = (union sctp_addr_param *)(asconf + 1);

	af = sctp_get_af_specific(param_type2af(param->p.type));
	if (unlikely(!af))
		return NULL;

	if (af->from_addr_param(&paddr, param, peer_port, 0))
		return NULL;

	return __sctp_lookup_association(net, laddr, &paddr, transportp);
}
