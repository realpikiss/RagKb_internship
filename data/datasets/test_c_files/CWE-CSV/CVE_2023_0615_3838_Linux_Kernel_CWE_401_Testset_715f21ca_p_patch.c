int vivid_vid_cap_s_selection(struct file *file, void *fh, struct v4l2_selection *s)
{
	struct vivid_dev *dev = video_drvdata(file);
	struct v4l2_rect *crop = &dev->crop_cap;
	struct v4l2_rect *compose = &dev->compose_cap;
	unsigned orig_compose_w = compose->width;
	unsigned orig_compose_h = compose->height;
	unsigned factor = V4L2_FIELD_HAS_T_OR_B(dev->field_cap) ? 2 : 1;
	int ret;

	if (!dev->has_crop_cap && !dev->has_compose_cap)
		return -ENOTTY;
	if (s->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	if (vivid_is_webcam(dev))
		return -ENODATA;

	switch (s->target) {
	case V4L2_SEL_TGT_CROP:
		if (!dev->has_crop_cap)
			return -EINVAL;
		ret = vivid_vid_adjust_sel(s->flags, &s->r);
		if (ret)
			return ret;
		v4l2_rect_set_min_size(&s->r, &vivid_min_rect);
		v4l2_rect_set_max_size(&s->r, &dev->src_rect);
		v4l2_rect_map_inside(&s->r, &dev->crop_bounds_cap);
		s->r.top /= factor;
		s->r.height /= factor;
		if (dev->has_scaler_cap) {
			struct v4l2_rect fmt = dev->fmt_cap_rect;
			struct v4l2_rect max_rect = {
				0, 0,
				s->r.width * MAX_ZOOM,
				s->r.height * MAX_ZOOM
			};
			struct v4l2_rect min_rect = {
				0, 0,
				s->r.width / MAX_ZOOM,
				s->r.height / MAX_ZOOM
			};

			v4l2_rect_set_min_size(&fmt, &min_rect);
			if (!dev->has_compose_cap)
				v4l2_rect_set_max_size(&fmt, &max_rect);
			if (!v4l2_rect_same_size(&dev->fmt_cap_rect, &fmt) &&
			    vb2_is_busy(&dev->vb_vid_cap_q))
				return -EBUSY;
			if (dev->has_compose_cap) {
				v4l2_rect_set_min_size(compose, &min_rect);
				v4l2_rect_set_max_size(compose, &max_rect);
			}
			dev->fmt_cap_rect = fmt;
			tpg_s_buf_height(&dev->tpg, fmt.height);
		} else if (dev->has_compose_cap) {
			struct v4l2_rect fmt = dev->fmt_cap_rect;

			v4l2_rect_set_min_size(&fmt, &s->r);
			if (!v4l2_rect_same_size(&dev->fmt_cap_rect, &fmt) &&
			    vb2_is_busy(&dev->vb_vid_cap_q))
				return -EBUSY;
			dev->fmt_cap_rect = fmt;
			tpg_s_buf_height(&dev->tpg, fmt.height);
			v4l2_rect_set_size_to(compose, &s->r);
			v4l2_rect_map_inside(compose, &dev->fmt_cap_rect);
		} else {
			if (!v4l2_rect_same_size(&s->r, &dev->fmt_cap_rect) &&
			    vb2_is_busy(&dev->vb_vid_cap_q))
				return -EBUSY;
			v4l2_rect_set_size_to(&dev->fmt_cap_rect, &s->r);
			v4l2_rect_set_size_to(compose, &s->r);
			v4l2_rect_map_inside(compose, &dev->fmt_cap_rect);
			tpg_s_buf_height(&dev->tpg, dev->fmt_cap_rect.height);
		}
		s->r.top *= factor;
		s->r.height *= factor;
		*crop = s->r;
		break;
	case V4L2_SEL_TGT_COMPOSE:
		if (!dev->has_compose_cap)
			return -EINVAL;
		ret = vivid_vid_adjust_sel(s->flags, &s->r);
		if (ret)
			return ret;
		v4l2_rect_set_min_size(&s->r, &vivid_min_rect);
		v4l2_rect_set_max_size(&s->r, &dev->fmt_cap_rect);
		if (dev->has_scaler_cap) {
			struct v4l2_rect max_rect = {
				0, 0,
				dev->src_rect.width * MAX_ZOOM,
				(dev->src_rect.height / factor) * MAX_ZOOM
			};

			v4l2_rect_set_max_size(&s->r, &max_rect);
			if (dev->has_crop_cap) {
				struct v4l2_rect min_rect = {
					0, 0,
					s->r.width / MAX_ZOOM,
					(s->r.height * factor) / MAX_ZOOM
				};
				struct v4l2_rect max_rect = {
					0, 0,
					s->r.width * MAX_ZOOM,
					(s->r.height * factor) * MAX_ZOOM
				};

				v4l2_rect_set_min_size(crop, &min_rect);
				v4l2_rect_set_max_size(crop, &max_rect);
				v4l2_rect_map_inside(crop, &dev->crop_bounds_cap);
			}
		} else if (dev->has_crop_cap) {
			s->r.top *= factor;
			s->r.height *= factor;
			v4l2_rect_set_max_size(&s->r, &dev->src_rect);
			v4l2_rect_set_size_to(crop, &s->r);
			v4l2_rect_map_inside(crop, &dev->crop_bounds_cap);
			s->r.top /= factor;
			s->r.height /= factor;
		} else {
			v4l2_rect_set_size_to(&s->r, &dev->src_rect);
			s->r.height /= factor;
		}
		v4l2_rect_map_inside(&s->r, &dev->fmt_cap_rect);
		*compose = s->r;
		break;
	default:
		return -EINVAL;
	}

	if (dev->bitmap_cap && (compose->width != orig_compose_w ||
				compose->height != orig_compose_h)) {
		vfree(dev->bitmap_cap);
		dev->bitmap_cap = NULL;
	}
	tpg_s_crop_compose(&dev->tpg, crop, compose);
	return 0;
}
