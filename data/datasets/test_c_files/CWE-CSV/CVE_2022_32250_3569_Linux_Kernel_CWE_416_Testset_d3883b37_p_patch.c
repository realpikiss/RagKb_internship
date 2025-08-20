static struct nft_expr *nft_expr_init(const struct nft_ctx *ctx,
				      const struct nlattr *nla)
{
	struct nft_expr_info expr_info;
	struct nft_expr *expr;
	struct module *owner;
	int err;

	err = nf_tables_expr_parse(ctx, nla, &expr_info);
	if (err < 0)
		goto err_expr_parse;

	err = -EOPNOTSUPP;
	if (!(expr_info.ops->type->flags & NFT_EXPR_STATEFUL))
		goto err_expr_stateful;

	err = -ENOMEM;
	expr = kzalloc(expr_info.ops->size, GFP_KERNEL_ACCOUNT);
	if (expr == NULL)
		goto err_expr_stateful;

	err = nf_tables_newexpr(ctx, &expr_info, expr);
	if (err < 0)
		goto err_expr_new;

	return expr;
err_expr_new:
	kfree(expr);
err_expr_stateful:
	owner = expr_info.ops->type->owner;
	if (expr_info.ops->type->release_ops)
		expr_info.ops->type->release_ops(expr_info.ops);

	module_put(owner);
err_expr_parse:
	return ERR_PTR(err);
}
