int nfc_llcp_data_received(struct nfc_dev *dev, struct sk_buff *skb)
{
	struct nfc_llcp_local *local;

	local = nfc_llcp_find_local(dev);
	if (local == NULL) {
		kfree_skb(skb);
		return -ENODEV;
	}

	__nfc_llcp_recv(local, skb);

	return 0;
}
