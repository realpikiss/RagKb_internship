static int vt_io_ioctl(struct vc_data *vc, unsigned int cmd, void __user *up,
		bool perm)
{
	switch (cmd) {
	case PIO_CMAP:
		if (!perm)
			return -EPERM;
		return con_set_cmap(up);

	case GIO_CMAP:
		return con_get_cmap(up);

	case PIO_SCRNMAP:
		if (!perm)
			return -EPERM;
		return con_set_trans_old(up);

	case GIO_SCRNMAP:
		return con_get_trans_old(up);

	case PIO_UNISCRNMAP:
		if (!perm)
			return -EPERM;
		return con_set_trans_new(up);

	case GIO_UNISCRNMAP:
		return con_get_trans_new(up);

	case PIO_UNIMAPCLR:
		if (!perm)
			return -EPERM;
		con_clear_unimap(vc);
		break;

	case PIO_UNIMAP:
	case GIO_UNIMAP:
		return do_unimap_ioctl(cmd, up, perm, vc);

	default:
		return -ENOIOCTLCMD;
	}

	return 0;
}