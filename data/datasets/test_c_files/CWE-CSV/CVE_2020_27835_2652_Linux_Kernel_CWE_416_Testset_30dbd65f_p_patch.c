static void unpin_sdma_pages(struct sdma_mmu_node *node)
{
	if (node->npages) {
		unpin_vector_pages(mm_from_sdma_node(node), node->pages, 0,
				   node->npages);
		atomic_sub(node->npages, &node->pq->n_locked);
	}
}
