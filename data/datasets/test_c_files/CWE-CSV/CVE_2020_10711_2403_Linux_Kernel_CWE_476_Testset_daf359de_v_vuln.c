static int cipso_v4_parsetag_rng(const struct cipso_v4_doi *doi_def,
				 const unsigned char *tag,
				 struct netlbl_lsm_secattr *secattr)
{
	int ret_val;
	u8 tag_len = tag[1];
	u32 level;

	ret_val = cipso_v4_map_lvl_ntoh(doi_def, tag[3], &level);
	if (ret_val != 0)
		return ret_val;
	secattr->attr.mls.lvl = level;
	secattr->flags |= NETLBL_SECATTR_MLS_LVL;

	if (tag_len > 4) {
		ret_val = cipso_v4_map_cat_rng_ntoh(doi_def,
						    &tag[4],
						    tag_len - 4,
						    secattr);
		if (ret_val != 0) {
			netlbl_catmap_free(secattr->attr.mls.cat);
			return ret_val;
		}

		secattr->flags |= NETLBL_SECATTR_MLS_CAT;
	}

	return 0;
}
