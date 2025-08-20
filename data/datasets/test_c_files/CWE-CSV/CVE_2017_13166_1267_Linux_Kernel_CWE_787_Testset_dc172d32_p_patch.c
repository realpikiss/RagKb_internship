static int v4l_enum_fmt(const struct v4l2_ioctl_ops *ops,
				struct file *file, void *fh, void *arg)
{
	struct v4l2_fmtdesc *p = arg;
	int ret = check_fmt(file, p->type);

	if (ret)
		return ret;
	ret = -EINVAL;

	switch (p->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		if (unlikely(!ops->vidioc_enum_fmt_vid_cap))
			break;
		ret = ops->vidioc_enum_fmt_vid_cap(file, fh, arg);
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		if (unlikely(!ops->vidioc_enum_fmt_vid_cap_mplane))
			break;
		ret = ops->vidioc_enum_fmt_vid_cap_mplane(file, fh, arg);
		break;
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
		if (unlikely(!ops->vidioc_enum_fmt_vid_overlay))
			break;
		ret = ops->vidioc_enum_fmt_vid_overlay(file, fh, arg);
		break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		if (unlikely(!ops->vidioc_enum_fmt_vid_out))
			break;
		ret = ops->vidioc_enum_fmt_vid_out(file, fh, arg);
		break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		if (unlikely(!ops->vidioc_enum_fmt_vid_out_mplane))
			break;
		ret = ops->vidioc_enum_fmt_vid_out_mplane(file, fh, arg);
		break;
	case V4L2_BUF_TYPE_SDR_CAPTURE:
		if (unlikely(!ops->vidioc_enum_fmt_sdr_cap))
			break;
		ret = ops->vidioc_enum_fmt_sdr_cap(file, fh, arg);
		break;
	case V4L2_BUF_TYPE_SDR_OUTPUT:
		if (unlikely(!ops->vidioc_enum_fmt_sdr_out))
			break;
		ret = ops->vidioc_enum_fmt_sdr_out(file, fh, arg);
		break;
	case V4L2_BUF_TYPE_META_CAPTURE:
		if (unlikely(!ops->vidioc_enum_fmt_meta_cap))
			break;
		ret = ops->vidioc_enum_fmt_meta_cap(file, fh, arg);
		break;
	}
	if (ret == 0)
		v4l_fill_fmtdesc(p);
	return ret;
}
