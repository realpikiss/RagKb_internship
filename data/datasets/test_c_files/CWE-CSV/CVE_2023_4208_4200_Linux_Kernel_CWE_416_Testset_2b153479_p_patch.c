static struct tc_u_knode *u32_init_knode(struct net *net, struct tcf_proto *tp,
					 struct tc_u_knode *n)
{
	struct tc_u_hnode *ht = rtnl_dereference(n->ht_down);
	struct tc_u32_sel *s = &n->sel;
	struct tc_u_knode *new;

	new = kzalloc(struct_size(new, sel.keys, s->nkeys), GFP_KERNEL);
	if (!new)
		return NULL;

	RCU_INIT_POINTER(new->next, n->next);
	new->handle = n->handle;
	RCU_INIT_POINTER(new->ht_up, n->ht_up);

	new->ifindex = n->ifindex;
	new->fshift = n->fshift;
	new->flags = n->flags;
	RCU_INIT_POINTER(new->ht_down, ht);

#ifdef CONFIG_CLS_U32_PERF
	/* Statistics may be incremented by readers during update
	 * so we must keep them in tact. When the node is later destroyed
	 * a special destroy call must be made to not free the pf memory.
	 */
	new->pf = n->pf;
#endif

#ifdef CONFIG_CLS_U32_MARK
	new->val = n->val;
	new->mask = n->mask;
	/* Similarly success statistics must be moved as pointers */
	new->pcpu_success = n->pcpu_success;
#endif
	memcpy(&new->sel, s, struct_size(s, keys, s->nkeys));

	if (tcf_exts_init(&new->exts, net, TCA_U32_ACT, TCA_U32_POLICE)) {
		kfree(new);
		return NULL;
	}

	/* bump reference count as long as we hold pointer to structure */
	if (ht)
		ht->refcnt++;

	return new;
}
