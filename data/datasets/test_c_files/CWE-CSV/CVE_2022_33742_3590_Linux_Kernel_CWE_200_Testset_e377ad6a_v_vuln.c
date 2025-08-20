static struct grant *get_indirect_grant(grant_ref_t *gref_head,
					struct blkfront_ring_info *rinfo)
{
	struct grant *gnt_list_entry = get_free_grant(rinfo);
	struct blkfront_info *info = rinfo->dev_info;

	if (gnt_list_entry->gref != INVALID_GRANT_REF)
		return gnt_list_entry;

	/* Assign a gref to this page */
	gnt_list_entry->gref = gnttab_claim_grant_reference(gref_head);
	BUG_ON(gnt_list_entry->gref == -ENOSPC);
	if (!info->feature_persistent) {
		struct page *indirect_page;

		/* Fetch a pre-allocated page to use for indirect grefs */
		BUG_ON(list_empty(&rinfo->indirect_pages));
		indirect_page = list_first_entry(&rinfo->indirect_pages,
						 struct page, lru);
		list_del(&indirect_page->lru);
		gnt_list_entry->page = indirect_page;
	}
	grant_foreign_access(gnt_list_entry, info);

	return gnt_list_entry;
}
