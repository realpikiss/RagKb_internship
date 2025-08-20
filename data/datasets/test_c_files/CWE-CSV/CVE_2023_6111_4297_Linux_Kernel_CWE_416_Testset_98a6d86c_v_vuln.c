static void nft_setelem_catchall_remove(const struct net *net,
					const struct nft_set *set,
					struct nft_elem_priv *elem_priv)
{
	struct nft_set_elem_catchall *catchall, *next;

	list_for_each_entry_safe(catchall, next, &set->catchall_list, list) {
		if (catchall->elem == elem_priv) {
			list_del_rcu(&catchall->list);
			kfree_rcu(catchall, rcu);
			break;
		}
	}
}
