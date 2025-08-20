static void nft_immediate_activate(const struct nft_ctx *ctx,
				   const struct nft_expr *expr)
{
	const struct nft_immediate_expr *priv = nft_expr_priv(expr);
	const struct nft_data *data = &priv->data;
	struct nft_ctx chain_ctx;
	struct nft_chain *chain;
	struct nft_rule *rule;

	if (priv->dreg == NFT_REG_VERDICT) {
		switch (data->verdict.code) {
		case NFT_JUMP:
		case NFT_GOTO:
			chain = data->verdict.chain;
			if (!nft_chain_binding(chain))
				break;

			chain_ctx = *ctx;
			chain_ctx.chain = chain;

			list_for_each_entry(rule, &chain->rules, list)
				nft_rule_expr_activate(&chain_ctx, rule);

			nft_clear(ctx->net, chain);
			break;
		default:
			break;
		}
	}

	return nft_data_hold(&priv->data, nft_dreg_to_type(priv->dreg));
}
