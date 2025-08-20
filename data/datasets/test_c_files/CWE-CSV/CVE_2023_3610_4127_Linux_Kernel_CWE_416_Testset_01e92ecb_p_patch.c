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

		if (!nft_chain_binding(chain))
			break;

		/* Rule construction failed, but chain is already bound:
		 * let the transaction records release this chain and its rules.
		 */
		if (chain->bound) {
			chain->use--;
			break;
		}

		/* Rule has been deleted, release chain and its rules. */
		chain_ctx = *ctx;
		chain_ctx.chain = chain;

		chain->use--;
		list_for_each_entry_safe(rule, n, &chain->rules, list) {
			chain->use--;
			list_del(&rule->list);
			nf_tables_rule_destroy(&chain_ctx, rule);
		}
		nf_tables_chain_destroy(&chain_ctx);
		break;
	default:
		break;
	}
}
