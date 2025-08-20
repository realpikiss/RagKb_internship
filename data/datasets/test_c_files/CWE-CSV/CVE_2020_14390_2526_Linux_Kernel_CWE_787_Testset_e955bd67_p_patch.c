static unsigned long fbcon_getxy(struct vc_data *vc, unsigned long pos,
				 int *px, int *py)
{
	unsigned long ret;
	int x, y;

	if (pos >= vc->vc_origin && pos < vc->vc_scr_end) {
		unsigned long offset = (pos - vc->vc_origin) / 2;

		x = offset % vc->vc_cols;
		y = offset / vc->vc_cols;
		ret = pos + (vc->vc_cols - x) * 2;
	} else {
		/* Should not happen */
		x = y = 0;
		ret = vc->vc_origin;
	}
	if (px)
		*px = x;
	if (py)
		*py = y;
	return ret;
}
