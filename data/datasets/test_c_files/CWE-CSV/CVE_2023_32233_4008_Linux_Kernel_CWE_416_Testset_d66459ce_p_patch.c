static void nft_lookup_activate(const struct nft_ctx *ctx,
				const struct nft_expr *expr)
{
	struct nft_lookup *priv = nft_expr_priv(expr);

	nf_tables_activate_set(ctx, priv->set);
}
