static bool sctp_v4_from_addr_param(union sctp_addr *addr,
				    union sctp_addr_param *param,
				    __be16 port, int iif)
{
	if (ntohs(param->v4.param_hdr.length) < sizeof(struct sctp_ipv4addr_param))
		return false;

	addr->v4.sin_family = AF_INET;
	addr->v4.sin_port = port;
	addr->v4.sin_addr.s_addr = param->v4.addr.s_addr;
	memset(addr->v4.sin_zero, 0, sizeof(addr->v4.sin_zero));

	return true;
}
