static void remove_nats_in_journal(struct f2fs_sb_info *sbi)
{
	struct f2fs_nm_info *nm_i = NM_I(sbi);
	struct curseg_info *curseg = CURSEG_I(sbi, CURSEG_HOT_DATA);
	struct f2fs_journal *journal = curseg->journal;
	int i;

	down_write(&curseg->journal_rwsem);
	for (i = 0; i < nats_in_cursum(journal); i++) {
		struct nat_entry *ne;
		struct f2fs_nat_entry raw_ne;
		nid_t nid = le32_to_cpu(nid_in_journal(journal, i));

		if (f2fs_check_nid_range(sbi, nid))
			continue;

		raw_ne = nat_in_journal(journal, i);

		ne = __lookup_nat_cache(nm_i, nid);
		if (!ne) {
			ne = __alloc_nat_entry(nid, true);
			__init_nat_entry(nm_i, ne, &raw_ne, true);
		}

		/*
		 * if a free nat in journal has not been used after last
		 * checkpoint, we should remove it from available nids,
		 * since later we will add it again.
		 */
		if (!get_nat_flag(ne, IS_DIRTY) &&
				le32_to_cpu(raw_ne.block_addr) == NULL_ADDR) {
			spin_lock(&nm_i->nid_list_lock);
			nm_i->available_nids--;
			spin_unlock(&nm_i->nid_list_lock);
		}

		__set_nat_cache_dirty(nm_i, ne);
	}
	update_nats_in_cursum(journal, -i);
	up_write(&curseg->journal_rwsem);
}
