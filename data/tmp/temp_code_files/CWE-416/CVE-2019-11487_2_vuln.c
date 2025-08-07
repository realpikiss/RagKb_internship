static inline void pipe_buf_get(struct pipe_inode_info *pipe,
				struct pipe_buffer *buf)
{
	buf->ops->get(pipe, buf);
}