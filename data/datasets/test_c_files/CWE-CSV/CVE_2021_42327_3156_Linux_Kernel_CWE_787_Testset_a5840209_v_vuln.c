static ssize_t dp_link_settings_write(struct file *f, const char __user *buf,
				 size_t size, loff_t *pos)
{
	struct amdgpu_dm_connector *connector = file_inode(f)->i_private;
	struct dc_link *link = connector->dc_link;
	struct dc_link_settings prefer_link_settings;
	char *wr_buf = NULL;
	const uint32_t wr_buf_size = 40;
	/* 0: lane_count; 1: link_rate */
	int max_param_num = 2;
	uint8_t param_nums = 0;
	long param[2];
	bool valid_input = true;

	if (size == 0)
		return -EINVAL;

	wr_buf = kcalloc(wr_buf_size, sizeof(char), GFP_KERNEL);
	if (!wr_buf)
		return -ENOSPC;

	if (parse_write_buffer_into_params(wr_buf, size,
					   (long *)param, buf,
					   max_param_num,
					   &param_nums)) {
		kfree(wr_buf);
		return -EINVAL;
	}

	if (param_nums <= 0) {
		kfree(wr_buf);
		DRM_DEBUG_DRIVER("user data not be read\n");
		return -EINVAL;
	}

	switch (param[0]) {
	case LANE_COUNT_ONE:
	case LANE_COUNT_TWO:
	case LANE_COUNT_FOUR:
		break;
	default:
		valid_input = false;
		break;
	}

	switch (param[1]) {
	case LINK_RATE_LOW:
	case LINK_RATE_HIGH:
	case LINK_RATE_RBR2:
	case LINK_RATE_HIGH2:
	case LINK_RATE_HIGH3:
		break;
	default:
		valid_input = false;
		break;
	}

	if (!valid_input) {
		kfree(wr_buf);
		DRM_DEBUG_DRIVER("Invalid Input value No HW will be programmed\n");
		return size;
	}

	/* save user force lane_count, link_rate to preferred settings
	 * spread spectrum will not be changed
	 */
	prefer_link_settings.link_spread = link->cur_link_settings.link_spread;
	prefer_link_settings.use_link_rate_set = false;
	prefer_link_settings.lane_count = param[0];
	prefer_link_settings.link_rate = param[1];

	dp_retrain_link_dp_test(link, &prefer_link_settings, false);

	kfree(wr_buf);
	return size;
}
