static inline int __bpf_skb_change_head(struct sk_buff *skb, u32 head_room,
					u64 flags)
{
	u32 max_len = __bpf_skb_max_len(skb);
	u32 new_len = skb->len + head_room;
	int ret;

	if (unlikely(flags || (!skb_is_gso(skb) && new_len > max_len) ||
		     new_len < skb->len))
		return -EINVAL;

	ret = skb_cow(skb, head_room);
	if (likely(!ret)) {
		/* Idea for this helper is that we currently only
		 * allow to expand on mac header. This means that
		 * skb->protocol network header, etc, stay as is.
		 * Compared to bpf_skb_change_tail(), we're more
		 * flexible due to not needing to linearize or
		 * reset GSO. Intention for this helper is to be
		 * used by an L3 skb that needs to push mac header
		 * for redirection into L2 device.
		 */
		__skb_push(skb, head_room);
		memset(skb->data, 0, head_room);
		skb_reset_mac_header(skb);
	}

	return ret;
}
