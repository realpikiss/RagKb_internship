static void ll_free_user_pages(struct page **pages, int npages, int do_dirty)
{
	int i;

	for (i = 0; i < npages; i++) {
		if (do_dirty)
			set_page_dirty_lock(pages[i]);
		page_cache_release(pages[i]);
	}
	kvfree(pages);
}
