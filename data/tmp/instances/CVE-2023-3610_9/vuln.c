static int nft_chain_add(struct nft_table *table, struct nft_chain *chain)
{
	int err;

	err = rhltable_insert_key(&table->chains_ht, chain->name,
				  &chain->rhlhead, nft_chain_ht_params);
	if (err)
		return err;

	list_add_tail_rcu(&chain->list, &table->chains);

	return 0;
}