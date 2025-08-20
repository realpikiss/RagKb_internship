static void netvsc_get_ethtool_stats(struct net_device *dev,
				     struct ethtool_stats *stats, u64 *data)
{
	struct net_device_context *ndc = netdev_priv(dev);
	struct netvsc_device *nvdev = rtnl_dereference(ndc->nvdev);
	const void *nds = &ndc->eth_stats;
	const struct netvsc_stats *qstats;
	struct netvsc_vf_pcpu_stats sum;
	struct netvsc_ethtool_pcpu_stats *pcpu_sum;
	unsigned int start;
	u64 packets, bytes;
	u64 xdp_drop;
	int i, j, cpu;

	if (!nvdev)
		return;

	for (i = 0; i < NETVSC_GLOBAL_STATS_LEN; i++)
		data[i] = *(unsigned long *)(nds + netvsc_stats[i].offset);

	netvsc_get_vf_stats(dev, &sum);
	for (j = 0; j < NETVSC_VF_STATS_LEN; j++)
		data[i++] = *(u64 *)((void *)&sum + vf_stats[j].offset);

	for (j = 0; j < nvdev->num_chn; j++) {
		qstats = &nvdev->chan_table[j].tx_stats;

		do {
			start = u64_stats_fetch_begin_irq(&qstats->syncp);
			packets = qstats->packets;
			bytes = qstats->bytes;
		} while (u64_stats_fetch_retry_irq(&qstats->syncp, start));
		data[i++] = packets;
		data[i++] = bytes;

		qstats = &nvdev->chan_table[j].rx_stats;
		do {
			start = u64_stats_fetch_begin_irq(&qstats->syncp);
			packets = qstats->packets;
			bytes = qstats->bytes;
			xdp_drop = qstats->xdp_drop;
		} while (u64_stats_fetch_retry_irq(&qstats->syncp, start));
		data[i++] = packets;
		data[i++] = bytes;
		data[i++] = xdp_drop;
	}

	pcpu_sum = kvmalloc_array(num_possible_cpus(),
				  sizeof(struct netvsc_ethtool_pcpu_stats),
				  GFP_KERNEL);
	if (!pcpu_sum)
		return;

	netvsc_get_pcpu_stats(dev, pcpu_sum);
	for_each_present_cpu(cpu) {
		struct netvsc_ethtool_pcpu_stats *this_sum = &pcpu_sum[cpu];

		for (j = 0; j < ARRAY_SIZE(pcpu_stats); j++)
			data[i++] = *(u64 *)((void *)this_sum
					     + pcpu_stats[j].offset);
	}
	kvfree(pcpu_sum);
}
