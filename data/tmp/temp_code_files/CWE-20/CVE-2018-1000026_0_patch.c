static netdev_features_t bnx2x_features_check(struct sk_buff *skb,
					      struct net_device *dev,
					      netdev_features_t features)
{
	/*
	 * A skb with gso_size + header length > 9700 will cause a
	 * firmware panic. Drop GSO support.
	 *
	 * Eventually the upper layer should not pass these packets down.
	 *
	 * For speed, if the gso_size is <= 9000, assume there will
	 * not be 700 bytes of headers and pass it through. Only do a
	 * full (slow) validation if the gso_size is > 9000.
	 *
	 * (Due to the way SKB_BY_FRAGS works this will also do a full
	 * validation in that case.)
	 */
	if (unlikely(skb_is_gso(skb) &&
		     (skb_shinfo(skb)->gso_size > 9000) &&
		     !skb_gso_validate_mac_len(skb, 9700)))
		features &= ~NETIF_F_GSO_MASK;

	features = vlan_features_check(skb, features);
	return vxlan_features_check(skb, features);
}