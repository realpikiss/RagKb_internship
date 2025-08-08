static int pfkey_dump(struct sock *sk, struct sk_buff *skb, const struct sadb_msg *hdr, void * const *ext_hdrs)
{
	u8 proto;
	struct xfrm_address_filter *filter = NULL;
	struct pfkey_sock *pfk = pfkey_sk(sk);

	mutex_lock(&pfk->dump_lock);
	if (pfk->dump.dump != NULL) {
		mutex_unlock(&pfk->dump_lock);
		return -EBUSY;
	}

	proto = pfkey_satype2proto(hdr->sadb_msg_satype);
	if (proto == 0) {
		mutex_unlock(&pfk->dump_lock);
		return -EINVAL;
	}

	if (ext_hdrs[SADB_X_EXT_FILTER - 1]) {
		struct sadb_x_filter *xfilter = ext_hdrs[SADB_X_EXT_FILTER - 1];

		filter = kmalloc(sizeof(*filter), GFP_KERNEL);
		if (filter == NULL) {
			mutex_unlock(&pfk->dump_lock);
			return -ENOMEM;
		}

		memcpy(&filter->saddr, &xfilter->sadb_x_filter_saddr,
		       sizeof(xfrm_address_t));
		memcpy(&filter->daddr, &xfilter->sadb_x_filter_daddr,
		       sizeof(xfrm_address_t));
		filter->family = xfilter->sadb_x_filter_family;
		filter->splen = xfilter->sadb_x_filter_splen;
		filter->dplen = xfilter->sadb_x_filter_dplen;
	}

	pfk->dump.msg_version = hdr->sadb_msg_version;
	pfk->dump.msg_portid = hdr->sadb_msg_pid;
	pfk->dump.dump = pfkey_dump_sa;
	pfk->dump.done = pfkey_dump_sa_done;
	xfrm_state_walk_init(&pfk->dump.u.state, proto, filter);
	mutex_unlock(&pfk->dump_lock);

	return pfkey_do_dump(pfk);
}