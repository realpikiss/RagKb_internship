static ssize_t sysfs_blk_trace_attr_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct hd_struct *p = dev_to_part(dev);
	struct request_queue *q;
	struct block_device *bdev;
	ssize_t ret = -ENXIO;

	bdev = bdget(part_devt(p));
	if (bdev == NULL)
		goto out;

	q = blk_trace_get_queue(bdev);
	if (q == NULL)
		goto out_bdput;

	mutex_lock(&q->blk_trace_mutex);

	if (attr == &dev_attr_enable) {
		ret = sprintf(buf, "%u\n", !!q->blk_trace);
		goto out_unlock_bdev;
	}

	if (q->blk_trace == NULL)
		ret = sprintf(buf, "disabled\n");
	else if (attr == &dev_attr_act_mask)
		ret = blk_trace_mask2str(buf, q->blk_trace->act_mask);
	else if (attr == &dev_attr_pid)
		ret = sprintf(buf, "%u\n", q->blk_trace->pid);
	else if (attr == &dev_attr_start_lba)
		ret = sprintf(buf, "%llu\n", q->blk_trace->start_lba);
	else if (attr == &dev_attr_end_lba)
		ret = sprintf(buf, "%llu\n", q->blk_trace->end_lba);

out_unlock_bdev:
	mutex_unlock(&q->blk_trace_mutex);
out_bdput:
	bdput(bdev);
out:
	return ret;
}
