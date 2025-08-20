static int valid_master_desc(const char *new_desc, const char *orig_desc)
{
	int prefix_len;

	if (!strncmp(new_desc, KEY_TRUSTED_PREFIX, KEY_TRUSTED_PREFIX_LEN))
		prefix_len = KEY_TRUSTED_PREFIX_LEN;
	else if (!strncmp(new_desc, KEY_USER_PREFIX, KEY_USER_PREFIX_LEN))
		prefix_len = KEY_USER_PREFIX_LEN;
	else
		return -EINVAL;

	if (!new_desc[prefix_len])
		return -EINVAL;

	if (orig_desc && strncmp(new_desc, orig_desc, prefix_len))
		return -EINVAL;

	return 0;
}
