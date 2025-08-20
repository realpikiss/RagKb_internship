static void nft_dynset_activate(const struct nft_ctx *ctx,
				const struct nft_expr *expr)
{
	struct nft_dynset *priv = nft_expr_priv(expr);

	priv->set->use++;
}
