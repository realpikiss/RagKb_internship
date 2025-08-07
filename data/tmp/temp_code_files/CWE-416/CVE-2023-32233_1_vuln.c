void nf_tables_deactivate_set(const struct nft_ctx *ctx, struct nft_set *set,
			      struct nft_set_binding *binding,
			      enum nft_trans_phase phase)
{
	switch (phase) {
	case NFT_TRANS_PREPARE:
		set->use--;
		return;
	case NFT_TRANS_ABORT:
	case NFT_TRANS_RELEASE:
		set->use--;
		fallthrough;
	default:
		nf_tables_unbind_set(ctx, set, binding,
				     phase == NFT_TRANS_COMMIT);
	}
}