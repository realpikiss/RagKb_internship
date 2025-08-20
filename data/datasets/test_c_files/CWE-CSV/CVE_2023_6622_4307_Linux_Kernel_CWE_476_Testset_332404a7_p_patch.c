static int nft_dynset_init(const struct nft_ctx *ctx,
			   const struct nft_expr *expr,
			   const struct nlattr * const tb[])
{
	struct nftables_pernet *nft_net = nft_pernet(ctx->net);
	struct nft_dynset *priv = nft_expr_priv(expr);
	u8 genmask = nft_genmask_next(ctx->net);
	struct nft_set *set;
	u64 timeout;
	int err, i;

	lockdep_assert_held(&nft_net->commit_mutex);

	if (tb[NFTA_DYNSET_SET_NAME] == NULL ||
	    tb[NFTA_DYNSET_OP] == NULL ||
	    tb[NFTA_DYNSET_SREG_KEY] == NULL)
		return -EINVAL;

	if (tb[NFTA_DYNSET_FLAGS]) {
		u32 flags = ntohl(nla_get_be32(tb[NFTA_DYNSET_FLAGS]));
		if (flags & ~(NFT_DYNSET_F_INV | NFT_DYNSET_F_EXPR))
			return -EOPNOTSUPP;
		if (flags & NFT_DYNSET_F_INV)
			priv->invert = true;
		if (flags & NFT_DYNSET_F_EXPR)
			priv->expr = true;
	}

	set = nft_set_lookup_global(ctx->net, ctx->table,
				    tb[NFTA_DYNSET_SET_NAME],
				    tb[NFTA_DYNSET_SET_ID], genmask);
	if (IS_ERR(set))
		return PTR_ERR(set);

	if (set->flags & NFT_SET_OBJECT)
		return -EOPNOTSUPP;

	if (set->ops->update == NULL)
		return -EOPNOTSUPP;

	if (set->flags & NFT_SET_CONSTANT)
		return -EBUSY;

	priv->op = ntohl(nla_get_be32(tb[NFTA_DYNSET_OP]));
	if (priv->op > NFT_DYNSET_OP_DELETE)
		return -EOPNOTSUPP;

	timeout = 0;
	if (tb[NFTA_DYNSET_TIMEOUT] != NULL) {
		if (!(set->flags & NFT_SET_TIMEOUT))
			return -EOPNOTSUPP;

		err = nf_msecs_to_jiffies64(tb[NFTA_DYNSET_TIMEOUT], &timeout);
		if (err)
			return err;
	}

	err = nft_parse_register_load(tb[NFTA_DYNSET_SREG_KEY], &priv->sreg_key,
				      set->klen);
	if (err < 0)
		return err;

	if (tb[NFTA_DYNSET_SREG_DATA] != NULL) {
		if (!(set->flags & NFT_SET_MAP))
			return -EOPNOTSUPP;
		if (set->dtype == NFT_DATA_VERDICT)
			return -EOPNOTSUPP;

		err = nft_parse_register_load(tb[NFTA_DYNSET_SREG_DATA],
					      &priv->sreg_data, set->dlen);
		if (err < 0)
			return err;
	} else if (set->flags & NFT_SET_MAP)
		return -EINVAL;

	if ((tb[NFTA_DYNSET_EXPR] || tb[NFTA_DYNSET_EXPRESSIONS]) &&
	    !(set->flags & NFT_SET_EVAL))
		return -EINVAL;

	if (tb[NFTA_DYNSET_EXPR]) {
		struct nft_expr *dynset_expr;

		dynset_expr = nft_dynset_expr_alloc(ctx, set,
						    tb[NFTA_DYNSET_EXPR], 0);
		if (IS_ERR(dynset_expr))
			return PTR_ERR(dynset_expr);

		priv->num_exprs++;
		priv->expr_array[0] = dynset_expr;

		if (set->num_exprs > 1 ||
		    (set->num_exprs == 1 &&
		     dynset_expr->ops != set->exprs[0]->ops)) {
			err = -EOPNOTSUPP;
			goto err_expr_free;
		}
	} else if (tb[NFTA_DYNSET_EXPRESSIONS]) {
		struct nft_expr *dynset_expr;
		struct nlattr *tmp;
		int left;

		if (!priv->expr)
			return -EINVAL;

		i = 0;
		nla_for_each_nested(tmp, tb[NFTA_DYNSET_EXPRESSIONS], left) {
			if (i == NFT_SET_EXPR_MAX) {
				err = -E2BIG;
				goto err_expr_free;
			}
			if (nla_type(tmp) != NFTA_LIST_ELEM) {
				err = -EINVAL;
				goto err_expr_free;
			}
			dynset_expr = nft_dynset_expr_alloc(ctx, set, tmp, i);
			if (IS_ERR(dynset_expr)) {
				err = PTR_ERR(dynset_expr);
				goto err_expr_free;
			}
			priv->expr_array[i] = dynset_expr;
			priv->num_exprs++;

			if (set->num_exprs) {
				if (i >= set->num_exprs) {
					err = -EINVAL;
					goto err_expr_free;
				}
				if (dynset_expr->ops != set->exprs[i]->ops) {
					err = -EOPNOTSUPP;
					goto err_expr_free;
				}
			}
			i++;
		}
		if (set->num_exprs && set->num_exprs != i) {
			err = -EOPNOTSUPP;
			goto err_expr_free;
		}
	} else if (set->num_exprs > 0) {
		err = nft_set_elem_expr_clone(ctx, set, priv->expr_array);
		if (err < 0)
			return err;

		priv->num_exprs = set->num_exprs;
	}

	nft_set_ext_prepare(&priv->tmpl);
	nft_set_ext_add_length(&priv->tmpl, NFT_SET_EXT_KEY, set->klen);
	if (set->flags & NFT_SET_MAP)
		nft_set_ext_add_length(&priv->tmpl, NFT_SET_EXT_DATA, set->dlen);

	if (priv->num_exprs)
		nft_dynset_ext_add_expr(priv);

	if (set->flags & NFT_SET_TIMEOUT) {
		if (timeout || set->timeout) {
			nft_set_ext_add(&priv->tmpl, NFT_SET_EXT_TIMEOUT);
			nft_set_ext_add(&priv->tmpl, NFT_SET_EXT_EXPIRATION);
		}
	}

	priv->timeout = timeout;

	err = nf_tables_bind_set(ctx, set, &priv->binding);
	if (err < 0)
		goto err_expr_free;

	if (set->size == 0)
		set->size = 0xffff;

	priv->set = set;
	return 0;

err_expr_free:
	for (i = 0; i < priv->num_exprs; i++)
		nft_expr_destroy(ctx, priv->expr_array[i]);
	return err;
}
