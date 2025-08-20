static void ll_free_user_pages(struct page **pages, int npages, int do_dirty)
{
	int i;

	for (i = 0; i < npages; i++) {
		if (pages[i] == NULL)
			break;
		if (do_dirty)
			set_page_dirty_lock(pages[i]);
		page_cache_release(pages[i]);
	}

	OBD_FREE_LARGE(pages, npages * sizeof(*pages));
}
