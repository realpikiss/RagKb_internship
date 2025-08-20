void nfc_llcp_unregister_device(struct nfc_dev *dev)
{
	struct nfc_llcp_local *local = nfc_llcp_find_local(dev);

	if (local == NULL) {
		pr_debug("No such device\n");
		return;
	}

	local_cleanup(local);

	nfc_llcp_local_put(local);
}
