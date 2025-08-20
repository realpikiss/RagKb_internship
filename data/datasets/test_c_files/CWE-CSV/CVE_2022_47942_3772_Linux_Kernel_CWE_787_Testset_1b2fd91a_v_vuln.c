static int smb2_calc_max_out_buf_len(struct ksmbd_work *work,
				     unsigned short hdr2_len,
				     unsigned int out_buf_len)
{
	int free_len;

	if (out_buf_len > work->conn->vals->max_trans_size)
		return -EINVAL;

	free_len = (int)(work->response_sz -
			 (get_rfc1002_len(work->response_buf) + 4)) -
		hdr2_len;
	if (free_len < 0)
		return -EINVAL;

	return min_t(int, out_buf_len, free_len);
}
