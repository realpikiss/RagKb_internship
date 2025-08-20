static int blkif_completion(unsigned long *id,
			    struct blkfront_ring_info *rinfo,
			    struct blkif_response *bret)
{
	int i = 0;
	struct scatterlist *sg;
	int num_sg, num_grant;
	struct blkfront_info *info = rinfo->dev_info;
	struct blk_shadow *s = &rinfo->shadow[*id];
	struct copy_from_grant data = {
		.grant_idx = 0,
	};

	num_grant = s->req.operation == BLKIF_OP_INDIRECT ?
		s->req.u.indirect.nr_segments : s->req.u.rw.nr_segments;

	/* The I/O request may be split in two. */
	if (unlikely(s->associated_id != NO_ASSOCIATED_ID)) {
		struct blk_shadow *s2 = &rinfo->shadow[s->associated_id];

		/* Keep the status of the current response in shadow. */
		s->status = blkif_rsp_to_req_status(bret->status);

		/* Wait the second response if not yet here. */
		if (s2->status < REQ_DONE)
			return 0;

		bret->status = blkif_get_final_status(s->status,
						      s2->status);

		/*
		 * All the grants is stored in the first shadow in order
		 * to make the completion code simpler.
		 */
		num_grant += s2->req.u.rw.nr_segments;

		/*
		 * The two responses may not come in order. Only the
		 * first request will store the scatter-gather list.
		 */
		if (s2->num_sg != 0) {
			/* Update "id" with the ID of the first response. */
			*id = s->associated_id;
			s = s2;
		}

		/*
		 * We don't need anymore the second request, so recycling
		 * it now.
		 */
		if (add_id_to_freelist(rinfo, s->associated_id))
			WARN(1, "%s: can't recycle the second part (id = %ld) of the request\n",
			     info->gd->disk_name, s->associated_id);
	}

	data.s = s;
	num_sg = s->num_sg;

	if (bret->operation == BLKIF_OP_READ && info->bounce) {
		for_each_sg(s->sg, sg, num_sg, i) {
			BUG_ON(sg->offset + sg->length > PAGE_SIZE);

			data.bvec_offset = sg->offset;
			data.bvec_data = kmap_atomic(sg_page(sg));

			gnttab_foreach_grant_in_range(sg_page(sg),
						      sg->offset,
						      sg->length,
						      blkif_copy_from_grant,
						      &data);

			kunmap_atomic(data.bvec_data);
		}
	}
	/* Add the persistent grant into the list of free grants */
	for (i = 0; i < num_grant; i++) {
		if (!gnttab_try_end_foreign_access(s->grants_used[i]->gref)) {
			/*
			 * If the grant is still mapped by the backend (the
			 * backend has chosen to make this grant persistent)
			 * we add it at the head of the list, so it will be
			 * reused first.
			 */
			if (!info->feature_persistent) {
				pr_alert("backed has not unmapped grant: %u\n",
					 s->grants_used[i]->gref);
				return -1;
			}
			list_add(&s->grants_used[i]->node, &rinfo->grants);
			rinfo->persistent_gnts_c++;
		} else {
			/*
			 * If the grant is not mapped by the backend we add it
			 * to the tail of the list, so it will not be picked
			 * again unless we run out of persistent grants.
			 */
			s->grants_used[i]->gref = INVALID_GRANT_REF;
			list_add_tail(&s->grants_used[i]->node, &rinfo->grants);
		}
	}
	if (s->req.operation == BLKIF_OP_INDIRECT) {
		for (i = 0; i < INDIRECT_GREFS(num_grant); i++) {
			if (!gnttab_try_end_foreign_access(s->indirect_grants[i]->gref)) {
				if (!info->feature_persistent) {
					pr_alert("backed has not unmapped grant: %u\n",
						 s->indirect_grants[i]->gref);
					return -1;
				}
				list_add(&s->indirect_grants[i]->node, &rinfo->grants);
				rinfo->persistent_gnts_c++;
			} else {
				struct page *indirect_page;

				/*
				 * Add the used indirect page back to the list of
				 * available pages for indirect grefs.
				 */
				if (!info->bounce) {
					indirect_page = s->indirect_grants[i]->page;
					list_add(&indirect_page->lru, &rinfo->indirect_pages);
				}
				s->indirect_grants[i]->gref = INVALID_GRANT_REF;
				list_add_tail(&s->indirect_grants[i]->node, &rinfo->grants);
			}
		}
	}

	return 1;
}
