static int v4l_enum_fmt(const struct v4l2_ioctl_ops *ops,
				struct file *file, void *fh, void *arg)
{
	struct v4l2_fmtdesc *p = arg;
	struct video_device *vfd = video_devdata(file);
	bool is_vid = vfd->vfl_type == VFL_TYPE_GRABBER;
	bool is_sdr = vfd->vfl_type == VFL_TYPE_SDR;
	bool is_tch = vfd->vfl_type == VFL_TYPE_TOUCH;
	bool is_rx = vfd->vfl_dir != VFL_DIR_TX;
	bool is_tx = vfd->vfl_dir != VFL_DIR_RX;
	int ret = -EINVAL;

	switch (p->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		if (unlikely(!is_rx || (!is_vid && !is_tch) || !ops->vidioc_enum_fmt_vid_cap))
			break;
		ret = ops->vidioc_enum_fmt_vid_cap(file, fh, arg);
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		if (unlikely(!is_rx || !is_vid || !ops->vidioc_enum_fmt_vid_cap_mplane))
			break;
		ret = ops->vidioc_enum_fmt_vid_cap_mplane(file, fh, arg);
		break;
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
		if (unlikely(!is_rx || !is_vid || !ops->vidioc_enum_fmt_vid_overlay))
			break;
		ret = ops->vidioc_enum_fmt_vid_overlay(file, fh, arg);
		break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		if (unlikely(!is_tx || !is_vid || !ops->vidioc_enum_fmt_vid_out))
			break;
		ret = ops->vidioc_enum_fmt_vid_out(file, fh, arg);
		break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		if (unlikely(!is_tx || !is_vid || !ops->vidioc_enum_fmt_vid_out_mplane))
			break;
		ret = ops->vidioc_enum_fmt_vid_out_mplane(file, fh, arg);
		break;
	case V4L2_BUF_TYPE_SDR_CAPTURE:
		if (unlikely(!is_rx || !is_sdr || !ops->vidioc_enum_fmt_sdr_cap))
			break;
		ret = ops->vidioc_enum_fmt_sdr_cap(file, fh, arg);
		break;
	case V4L2_BUF_TYPE_SDR_OUTPUT:
		if (unlikely(!is_tx || !is_sdr || !ops->vidioc_enum_fmt_sdr_out))
			break;
		ret = ops->vidioc_enum_fmt_sdr_out(file, fh, arg);
		break;
	case V4L2_BUF_TYPE_META_CAPTURE:
		if (unlikely(!is_rx || !is_vid || !ops->vidioc_enum_fmt_meta_cap))
			break;
		ret = ops->vidioc_enum_fmt_meta_cap(file, fh, arg);
		break;
	}
	if (ret == 0)
		v4l_fill_fmtdesc(p);
	return ret;
}
