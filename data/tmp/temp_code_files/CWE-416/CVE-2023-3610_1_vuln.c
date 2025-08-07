void nft_data_hold(const struct nft_data *data, enum nft_data_types type)
{
	struct nft_chain *chain;
	struct nft_rule *rule;

	if (type == NFT_DATA_VERDICT) {
		switch (data->verdict.code) {
		case NFT_JUMP:
		case NFT_GOTO:
			chain = data->verdict.chain;
			chain->use++;

			if (!nft_chain_is_bound(chain))
				break;

			chain->table->use++;
			list_for_each_entry(rule, &chain->rules, list)
				chain->use++;

			nft_chain_add(chain->table, chain);
			break;
		}
	}
}