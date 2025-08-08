static void nft_verdict_uninit(const struct nft_data *data)
{
	struct nft_chain *chain;
	struct nft_rule *rule;

	switch (data->verdict.code) {
	case NFT_JUMP:
	case NFT_GOTO:
		chain = data->verdict.chain;
		chain->use--;

		if (!nft_chain_is_bound(chain))
			break;

		chain->table->use--;
		list_for_each_entry(rule, &chain->rules, list)
			chain->use--;

		nft_chain_del(chain);
		break;
	}
}