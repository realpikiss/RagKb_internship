static int con_font_get(struct vc_data *vc, struct console_font_op *op)
{
	struct console_font font;
	int rc = -EINVAL;
	int c;

	if (op->data) {
		font.data = kmalloc(max_font_size, GFP_KERNEL);
		if (!font.data)
			return -ENOMEM;
	} else
		font.data = NULL;

	console_lock();
	if (vc->vc_mode != KD_TEXT)
		rc = -EINVAL;
	else if (vc->vc_sw->con_font_get)
		rc = vc->vc_sw->con_font_get(vc, &font);
	else
		rc = -ENOSYS;
	console_unlock();

	if (rc)
		goto out;

	c = (font.width+7)/8 * 32 * font.charcount;

	if (op->data && font.charcount > op->charcount)
		rc = -ENOSPC;
	if (font.width > op->width || font.height > op->height)
		rc = -ENOSPC;
	if (rc)
		goto out;

	op->height = font.height;
	op->width = font.width;
	op->charcount = font.charcount;

	if (op->data && copy_to_user(op->data, font.data, c))
		rc = -EFAULT;

out:
	kfree(font.data);
	return rc;
}
