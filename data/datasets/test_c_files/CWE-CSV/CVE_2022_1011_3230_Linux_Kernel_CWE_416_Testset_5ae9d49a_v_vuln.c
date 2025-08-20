static int fuse_copy_page(struct fuse_copy_state *cs, struct page **pagep,
			  unsigned offset, unsigned count, int zeroing)
{
	int err;
	struct page *page = *pagep;

	if (page && zeroing && count < PAGE_SIZE)
		clear_highpage(page);

	while (count) {
		if (cs->write && cs->pipebufs && page) {
			return fuse_ref_page(cs, page, offset, count);
		} else if (!cs->len) {
			if (cs->move_pages && page &&
			    offset == 0 && count == PAGE_SIZE) {
				err = fuse_try_move_page(cs, pagep);
				if (err <= 0)
					return err;
			} else {
				err = fuse_copy_fill(cs);
				if (err)
					return err;
			}
		}
		if (page) {
			void *mapaddr = kmap_local_page(page);
			void *buf = mapaddr + offset;
			offset += fuse_copy_do(cs, &buf, &count);
			kunmap_local(mapaddr);
		} else
			offset += fuse_copy_do(cs, NULL, &count);
	}
	if (page && !cs->write)
		flush_dcache_page(page);
	return 0;
}
