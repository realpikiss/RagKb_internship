static ssize_t sysfs_blk_trace_attr_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct hd_struct *p = dev_to_part(dev);
	struct request_queue *q;
	struct block_device *bdev;
	struct blk_trace *bt;
	ssize_t ret = -ENXIO;

	bdev = bdget(part_devt(p));
	if (bdev == NULL)
		goto out;

	q = blk_trace_get_queue(bdev);
	if (q == NULL)
		goto out_bdput;

	mutex_lock(&q->blk_trace_mutex);

	bt = rcu_dereference_protected(q->blk_trace,
				       lockdep_is_held(&q->blk_trace_mutex));
	if (attr == &dev_attr_enable) {
		ret = sprintf(buf, "%u\n", !!bt);
		goto out_unlock_bdev;
	}

	if (bt == NULL)
		ret = sprintf(buf, "disabled\n");
	else if (attr == &dev_attr_act_mask)
		ret = blk_trace_mask2str(buf, bt->act_mask);
	else if (attr == &dev_attr_pid)
		ret = sprintf(buf, "%u\n", bt->pid);
	else if (attr == &dev_attr_start_lba)
		ret = sprintf(buf, "%llu\n", bt->start_lba);
	else if (attr == &dev_attr_end_lba)
		ret = sprintf(buf, "%llu\n", bt->end_lba);

out_unlock_bdev:
	mutex_unlock(&q->blk_trace_mutex);
out_bdput:
	bdput(bdev);
out:
	return ret;
}
