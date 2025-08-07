static void nft_rule_expr_activate(const struct nft_ctx *ctx,
				   struct nft_rule *rule)
{
	struct nft_expr *expr;

	expr = nft_expr_first(rule);
	while (nft_expr_more(rule, expr)) {
		if (expr->ops->activate)
			expr->ops->activate(ctx, expr);

		expr = nft_expr_next(expr);
	}
}