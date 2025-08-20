int nfc_llcp_set_remote_gb(struct nfc_dev *dev, const u8 *gb, u8 gb_len)
{
	struct nfc_llcp_local *local;
	int err;

	if (gb_len < 3 || gb_len > NFC_MAX_GT_LEN)
		return -EINVAL;

	local = nfc_llcp_find_local(dev);
	if (local == NULL) {
		pr_err("No LLCP device\n");
		return -ENODEV;
	}

	memset(local->remote_gb, 0, NFC_MAX_GT_LEN);
	memcpy(local->remote_gb, gb, gb_len);
	local->remote_gb_len = gb_len;

	if (memcmp(local->remote_gb, llcp_magic, 3)) {
		pr_err("MAC does not support LLCP\n");
		err = -EINVAL;
		goto out;
	}

	err = nfc_llcp_parse_gb_tlv(local,
				     &local->remote_gb[3],
				     local->remote_gb_len - 3);
out:
	nfc_llcp_local_put(local);
	return err;
}
