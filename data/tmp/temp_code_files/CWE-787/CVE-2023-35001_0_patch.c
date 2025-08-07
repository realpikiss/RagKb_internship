void nft_byteorder_eval(const struct nft_expr *expr,
			struct nft_regs *regs,
			const struct nft_pktinfo *pkt)
{
	const struct nft_byteorder *priv = nft_expr_priv(expr);
	u32 *src = &regs->data[priv->sreg];
	u32 *dst = &regs->data[priv->dreg];
	u16 *s16, *d16;
	unsigned int i;

	s16 = (void *)src;
	d16 = (void *)dst;

	switch (priv->size) {
	case 8: {
		u64 src64;

		switch (priv->op) {
		case NFT_BYTEORDER_NTOH:
			for (i = 0; i < priv->len / 8; i++) {
				src64 = nft_reg_load64(&src[i]);
				nft_reg_store64(&dst[i],
						be64_to_cpu((__force __be64)src64));
			}
			break;
		case NFT_BYTEORDER_HTON:
			for (i = 0; i < priv->len / 8; i++) {
				src64 = (__force __u64)
					cpu_to_be64(nft_reg_load64(&src[i]));
				nft_reg_store64(&dst[i], src64);
			}
			break;
		}
		break;
	}
	case 4:
		switch (priv->op) {
		case NFT_BYTEORDER_NTOH:
			for (i = 0; i < priv->len / 4; i++)
				dst[i] = ntohl((__force __be32)src[i]);
			break;
		case NFT_BYTEORDER_HTON:
			for (i = 0; i < priv->len / 4; i++)
				dst[i] = (__force __u32)htonl(src[i]);
			break;
		}
		break;
	case 2:
		switch (priv->op) {
		case NFT_BYTEORDER_NTOH:
			for (i = 0; i < priv->len / 2; i++)
				d16[i] = ntohs((__force __be16)s16[i]);
			break;
		case NFT_BYTEORDER_HTON:
			for (i = 0; i < priv->len / 2; i++)
				d16[i] = (__force __u16)htons(s16[i]);
			break;
		}
		break;
	}
}