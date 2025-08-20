static int __nf_tables_abort(struct net *net, enum nfnl_abort_action action)
{
	struct nftables_pernet *nft_net = nft_pernet(net);
	struct nft_trans *trans, *next;
	LIST_HEAD(set_update_list);
	struct nft_trans_elem *te;

	if (action == NFNL_ABORT_VALIDATE &&
	    nf_tables_validate(net) < 0)
		return -EAGAIN;

	list_for_each_entry_safe_reverse(trans, next, &nft_net->commit_list,
					 list) {
		switch (trans->msg_type) {
		case NFT_MSG_NEWTABLE:
			if (nft_trans_table_update(trans)) {
				if (!(trans->ctx.table->flags & __NFT_TABLE_F_UPDATE)) {
					nft_trans_destroy(trans);
					break;
				}
				if (trans->ctx.table->flags & __NFT_TABLE_F_WAS_DORMANT) {
					nf_tables_table_disable(net, trans->ctx.table);
					trans->ctx.table->flags |= NFT_TABLE_F_DORMANT;
				} else if (trans->ctx.table->flags & __NFT_TABLE_F_WAS_AWAKEN) {
					trans->ctx.table->flags &= ~NFT_TABLE_F_DORMANT;
				}
				trans->ctx.table->flags &= ~__NFT_TABLE_F_UPDATE;
				nft_trans_destroy(trans);
			} else {
				list_del_rcu(&trans->ctx.table->list);
			}
			break;
		case NFT_MSG_DELTABLE:
		case NFT_MSG_DESTROYTABLE:
			nft_clear(trans->ctx.net, trans->ctx.table);
			nft_trans_destroy(trans);
			break;
		case NFT_MSG_NEWCHAIN:
			if (nft_trans_chain_update(trans)) {
				nft_netdev_unregister_hooks(net,
							    &nft_trans_chain_hooks(trans),
							    true);
				free_percpu(nft_trans_chain_stats(trans));
				kfree(nft_trans_chain_name(trans));
				nft_trans_destroy(trans);
			} else {
				if (nft_trans_chain_bound(trans)) {
					nft_trans_destroy(trans);
					break;
				}
				trans->ctx.table->use--;
				nft_chain_del(trans->ctx.chain);
				nf_tables_unregister_hook(trans->ctx.net,
							  trans->ctx.table,
							  trans->ctx.chain);
			}
			break;
		case NFT_MSG_DELCHAIN:
		case NFT_MSG_DESTROYCHAIN:
			if (nft_trans_chain_update(trans)) {
				list_splice(&nft_trans_chain_hooks(trans),
					    &nft_trans_basechain(trans)->hook_list);
			} else {
				trans->ctx.table->use++;
				nft_clear(trans->ctx.net, trans->ctx.chain);
			}
			nft_trans_destroy(trans);
			break;
		case NFT_MSG_NEWRULE:
			if (nft_trans_rule_bound(trans)) {
				nft_trans_destroy(trans);
				break;
			}
			trans->ctx.chain->use--;
			list_del_rcu(&nft_trans_rule(trans)->list);
			nft_rule_expr_deactivate(&trans->ctx,
						 nft_trans_rule(trans),
						 NFT_TRANS_ABORT);
			if (trans->ctx.chain->flags & NFT_CHAIN_HW_OFFLOAD)
				nft_flow_rule_destroy(nft_trans_flow_rule(trans));
			break;
		case NFT_MSG_DELRULE:
		case NFT_MSG_DESTROYRULE:
			trans->ctx.chain->use++;
			nft_clear(trans->ctx.net, nft_trans_rule(trans));
			nft_rule_expr_activate(&trans->ctx, nft_trans_rule(trans));
			if (trans->ctx.chain->flags & NFT_CHAIN_HW_OFFLOAD)
				nft_flow_rule_destroy(nft_trans_flow_rule(trans));

			nft_trans_destroy(trans);
			break;
		case NFT_MSG_NEWSET:
			if (nft_trans_set_update(trans)) {
				nft_trans_destroy(trans);
				break;
			}
			trans->ctx.table->use--;
			if (nft_trans_set_bound(trans)) {
				nft_trans_destroy(trans);
				break;
			}
			list_del_rcu(&nft_trans_set(trans)->list);
			break;
		case NFT_MSG_DELSET:
		case NFT_MSG_DESTROYSET:
			trans->ctx.table->use++;
			nft_clear(trans->ctx.net, nft_trans_set(trans));
			nft_trans_destroy(trans);
			break;
		case NFT_MSG_NEWSETELEM:
			if (nft_trans_elem_set_bound(trans)) {
				nft_trans_destroy(trans);
				break;
			}
			te = (struct nft_trans_elem *)trans->data;
			nft_setelem_remove(net, te->set, &te->elem);
			if (!nft_setelem_is_catchall(te->set, &te->elem))
				atomic_dec(&te->set->nelems);

			if (te->set->ops->abort &&
			    list_empty(&te->set->pending_update)) {
				list_add_tail(&te->set->pending_update,
					      &set_update_list);
			}
			break;
		case NFT_MSG_DELSETELEM:
		case NFT_MSG_DESTROYSETELEM:
			te = (struct nft_trans_elem *)trans->data;

			nft_setelem_data_activate(net, te->set, &te->elem);
			nft_setelem_activate(net, te->set, &te->elem);
			if (!nft_setelem_is_catchall(te->set, &te->elem))
				te->set->ndeact--;

			if (te->set->ops->abort &&
			    list_empty(&te->set->pending_update)) {
				list_add_tail(&te->set->pending_update,
					      &set_update_list);
			}
			nft_trans_destroy(trans);
			break;
		case NFT_MSG_NEWOBJ:
			if (nft_trans_obj_update(trans)) {
				nft_obj_destroy(&trans->ctx, nft_trans_obj_newobj(trans));
				nft_trans_destroy(trans);
			} else {
				trans->ctx.table->use--;
				nft_obj_del(nft_trans_obj(trans));
			}
			break;
		case NFT_MSG_DELOBJ:
		case NFT_MSG_DESTROYOBJ:
			trans->ctx.table->use++;
			nft_clear(trans->ctx.net, nft_trans_obj(trans));
			nft_trans_destroy(trans);
			break;
		case NFT_MSG_NEWFLOWTABLE:
			if (nft_trans_flowtable_update(trans)) {
				nft_unregister_flowtable_net_hooks(net,
						&nft_trans_flowtable_hooks(trans));
			} else {
				trans->ctx.table->use--;
				list_del_rcu(&nft_trans_flowtable(trans)->list);
				nft_unregister_flowtable_net_hooks(net,
						&nft_trans_flowtable(trans)->hook_list);
			}
			break;
		case NFT_MSG_DELFLOWTABLE:
		case NFT_MSG_DESTROYFLOWTABLE:
			if (nft_trans_flowtable_update(trans)) {
				list_splice(&nft_trans_flowtable_hooks(trans),
					    &nft_trans_flowtable(trans)->hook_list);
			} else {
				trans->ctx.table->use++;
				nft_clear(trans->ctx.net, nft_trans_flowtable(trans));
			}
			nft_trans_destroy(trans);
			break;
		}
	}

	nft_set_abort_update(&set_update_list);

	synchronize_rcu();

	list_for_each_entry_safe_reverse(trans, next,
					 &nft_net->commit_list, list) {
		list_del(&trans->list);
		nf_tables_abort_release(trans);
	}

	if (action == NFNL_ABORT_AUTOLOAD)
		nf_tables_module_autoload(net);
	else
		nf_tables_module_autoload_cleanup(net);

	return 0;
}
