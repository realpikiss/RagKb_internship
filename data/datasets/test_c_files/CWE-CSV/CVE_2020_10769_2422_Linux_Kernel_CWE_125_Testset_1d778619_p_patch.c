int crypto_authenc_extractkeys(struct crypto_authenc_keys *keys, const u8 *key,
			       unsigned int keylen)
{
	struct rtattr *rta = (struct rtattr *)key;
	struct crypto_authenc_key_param *param;

	if (!RTA_OK(rta, keylen))
		return -EINVAL;
	if (rta->rta_type != CRYPTO_AUTHENC_KEYA_PARAM)
		return -EINVAL;

	/*
	 * RTA_OK() didn't align the rtattr's payload when validating that it
	 * fits in the buffer.  Yet, the keys should start on the next 4-byte
	 * aligned boundary.  To avoid confusion, require that the rtattr
	 * payload be exactly the param struct, which has a 4-byte aligned size.
	 */
	if (RTA_PAYLOAD(rta) != sizeof(*param))
		return -EINVAL;
	BUILD_BUG_ON(sizeof(*param) % RTA_ALIGNTO);

	param = RTA_DATA(rta);
	keys->enckeylen = be32_to_cpu(param->enckeylen);

	key += rta->rta_len;
	keylen -= rta->rta_len;

	if (keylen < keys->enckeylen)
		return -EINVAL;

	keys->authkeylen = keylen - keys->enckeylen;
	keys->authkey = key;
	keys->enckey = key + keys->authkeylen;

	return 0;
}
