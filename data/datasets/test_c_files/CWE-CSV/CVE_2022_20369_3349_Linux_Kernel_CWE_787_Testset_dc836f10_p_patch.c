int v4l2_m2m_dqbuf(struct file *file, struct v4l2_m2m_ctx *m2m_ctx,
		   struct v4l2_buffer *buf)
{
	struct vb2_queue *vq;
	int ret;

	vq = v4l2_m2m_get_vq(m2m_ctx, buf->type);
	ret = vb2_dqbuf(vq, buf, file->f_flags & O_NONBLOCK);
	if (ret)
		return ret;

	/* Adjust MMAP memory offsets for the CAPTURE queue */
	v4l2_m2m_adjust_mem_offset(vq, buf);

	return 0;
}
