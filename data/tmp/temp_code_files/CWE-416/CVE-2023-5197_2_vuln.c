static int nft_flush_table(struct nft_ctx *ctx)
{
	struct nft_flowtable *flowtable, *nft;
	struct nft_chain *chain, *nc;
	struct nft_object *obj, *ne;
	struct nft_set *set, *ns;
	int err;

	list_for_each_entry(chain, &ctx->table->chains, list) {
		if (!nft_is_active_next(ctx->net, chain))
			continue;

		if (nft_chain_is_bound(chain))
			continue;

		ctx->chain = chain;

		err = nft_delrule_by_chain(ctx);
		if (err < 0)
			goto out;
	}

	list_for_each_entry_safe(set, ns, &ctx->table->sets, list) {
		if (!nft_is_active_next(ctx->net, set))
			continue;

		if (nft_set_is_anonymous(set) &&
		    !list_empty(&set->bindings))
			continue;

		err = nft_delset(ctx, set);
		if (err < 0)
			goto out;
	}

	list_for_each_entry_safe(flowtable, nft, &ctx->table->flowtables, list) {
		if (!nft_is_active_next(ctx->net, flowtable))
			continue;

		err = nft_delflowtable(ctx, flowtable);
		if (err < 0)
			goto out;
	}

	list_for_each_entry_safe(obj, ne, &ctx->table->objects, list) {
		if (!nft_is_active_next(ctx->net, obj))
			continue;

		err = nft_delobj(ctx, obj);
		if (err < 0)
			goto out;
	}

	list_for_each_entry_safe(chain, nc, &ctx->table->chains, list) {
		if (!nft_is_active_next(ctx->net, chain))
			continue;

		if (nft_chain_is_bound(chain))
			continue;

		ctx->chain = chain;

		err = nft_delchain(ctx);
		if (err < 0)
			goto out;
	}

	err = nft_deltable(ctx);
out:
	return err;
}