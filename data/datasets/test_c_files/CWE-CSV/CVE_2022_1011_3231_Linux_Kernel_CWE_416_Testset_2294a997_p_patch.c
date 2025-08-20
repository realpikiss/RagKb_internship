static int fuse_get_user_pages(struct fuse_args_pages *ap, struct iov_iter *ii,
			       size_t *nbytesp, int write,
			       unsigned int max_pages)
{
	size_t nbytes = 0;  /* # bytes already packed in req */
	ssize_t ret = 0;

	/* Special case for kernel I/O: can copy directly into the buffer */
	if (iov_iter_is_kvec(ii)) {
		unsigned long user_addr = fuse_get_user_addr(ii);
		size_t frag_size = fuse_get_frag_size(ii, *nbytesp);

		if (write)
			ap->args.in_args[1].value = (void *) user_addr;
		else
			ap->args.out_args[0].value = (void *) user_addr;

		iov_iter_advance(ii, frag_size);
		*nbytesp = frag_size;
		return 0;
	}

	while (nbytes < *nbytesp && ap->num_pages < max_pages) {
		unsigned npages;
		size_t start;
		ret = iov_iter_get_pages(ii, &ap->pages[ap->num_pages],
					*nbytesp - nbytes,
					max_pages - ap->num_pages,
					&start);
		if (ret < 0)
			break;

		iov_iter_advance(ii, ret);
		nbytes += ret;

		ret += start;
		npages = DIV_ROUND_UP(ret, PAGE_SIZE);

		ap->descs[ap->num_pages].offset = start;
		fuse_page_descs_length_init(ap->descs, ap->num_pages, npages);

		ap->num_pages += npages;
		ap->descs[ap->num_pages - 1].length -=
			(PAGE_SIZE - ret) & (PAGE_SIZE - 1);
	}

	ap->args.user_pages = true;
	if (write)
		ap->args.in_pages = true;
	else
		ap->args.out_pages = true;

	*nbytesp = nbytes;

	return ret < 0 ? ret : 0;
}
