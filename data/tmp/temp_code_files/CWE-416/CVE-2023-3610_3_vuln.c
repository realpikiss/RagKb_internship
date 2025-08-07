void nf_tables_rule_release(const struct nft_ctx *ctx, struct nft_rule *rule)
{
	nft_rule_expr_deactivate(ctx, rule, NFT_TRANS_RELEASE);
	nf_tables_rule_destroy(ctx, rule);
}