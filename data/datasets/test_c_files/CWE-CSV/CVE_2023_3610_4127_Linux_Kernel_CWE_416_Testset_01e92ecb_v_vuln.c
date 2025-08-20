static void nft_immediate_destroy(const struct nft_ctx *ctx,
				  const struct nft_expr *expr)
{
	const struct nft_immediate_expr *priv = nft_expr_priv(expr);
	const struct nft_data *data = &priv->data;
	struct nft_rule *rule, *n;
	struct nft_ctx chain_ctx;
	struct nft_chain *chain;

	if (priv->dreg != NFT_REG_VERDICT)
		return;

	switch (data->verdict.code) {
	case NFT_JUMP:
	case NFT_GOTO:
		chain = data->verdict.chain;

		if (!nft_chain_is_bound(chain))
			break;

		chain_ctx = *ctx;
		chain_ctx.chain = chain;

		list_for_each_entry_safe(rule, n, &chain->rules, list)
			nf_tables_rule_release(&chain_ctx, rule);

		nf_tables_chain_destroy(&chain_ctx);
		break;
	default:
		break;
	}
}
