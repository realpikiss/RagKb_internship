int sctp_bind_addr_copy(struct net *net, struct sctp_bind_addr *dest,
			const struct sctp_bind_addr *src,
			enum sctp_scope scope, gfp_t gfp,
			int flags)
{
	struct sctp_sockaddr_entry *addr;
	int error = 0;

	/* All addresses share the same port.  */
	dest->port = src->port;

	/* Extract the addresses which are relevant for this scope.  */
	list_for_each_entry(addr, &src->address_list, list) {
		error = sctp_copy_one_addr(net, dest, &addr->a, scope,
					   gfp, flags);
		if (error < 0)
			goto out;
	}

	/* If there are no addresses matching the scope and
	 * this is global scope, try to get a link scope address, with
	 * the assumption that we must be sitting behind a NAT.
	 */
	if (list_empty(&dest->address_list) && (SCTP_SCOPE_GLOBAL == scope)) {
		list_for_each_entry(addr, &src->address_list, list) {
			error = sctp_copy_one_addr(net, dest, &addr->a,
						   SCTP_SCOPE_LINK, gfp,
						   flags);
			if (error < 0)
				goto out;
		}
	}

	/* If somehow no addresses were found that can be used with this
	 * scope, it's an error.
	 */
	if (list_empty(&dest->address_list))
		error = -ENETUNREACH;

out:
	if (error)
		sctp_bind_addr_clean(dest);

	return error;
}
