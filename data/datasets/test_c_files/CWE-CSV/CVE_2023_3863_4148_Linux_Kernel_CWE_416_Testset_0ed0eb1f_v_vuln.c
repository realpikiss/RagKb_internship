u8 *nfc_llcp_general_bytes(struct nfc_dev *dev, size_t *general_bytes_len)
{
	struct nfc_llcp_local *local;

	local = nfc_llcp_find_local(dev);
	if (local == NULL) {
		*general_bytes_len = 0;
		return NULL;
	}

	nfc_llcp_build_gb(local);

	*general_bytes_len = local->gb_len;

	return local->gb;
}
