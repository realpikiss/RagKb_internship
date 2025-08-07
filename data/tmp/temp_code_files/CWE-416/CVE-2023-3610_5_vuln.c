static void nft_immediate_deactivate(const struct nft_ctx *ctx,
				     const struct nft_expr *expr,
				     enum nft_trans_phase phase)
{
	const struct nft_immediate_expr *priv = nft_expr_priv(expr);

	if (phase == NFT_TRANS_COMMIT)
		return;

	return nft_data_release(&priv->data, nft_dreg_to_type(priv->dreg));
}