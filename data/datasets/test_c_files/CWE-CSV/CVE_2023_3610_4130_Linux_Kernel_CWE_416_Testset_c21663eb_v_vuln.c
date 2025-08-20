static void nft_immediate_activate(const struct nft_ctx *ctx,
				   const struct nft_expr *expr)
{
	const struct nft_immediate_expr *priv = nft_expr_priv(expr);

	return nft_data_hold(&priv->data, nft_dreg_to_type(priv->dreg));
}
