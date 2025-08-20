static int kvm_create_vm_debugfs(struct kvm *kvm, int fd)
{
	char dir_name[ITOA_MAX_LEN * 2];
	struct kvm_stat_data *stat_data;
	struct kvm_stats_debugfs_item *p;

	if (!debugfs_initialized())
		return 0;

	snprintf(dir_name, sizeof(dir_name), "%d-%d", task_pid_nr(current), fd);
	kvm->debugfs_dentry = debugfs_create_dir(dir_name, kvm_debugfs_dir);

	kvm->debugfs_stat_data = kcalloc(kvm_debugfs_num_entries,
					 sizeof(*kvm->debugfs_stat_data),
					 GFP_KERNEL_ACCOUNT);
	if (!kvm->debugfs_stat_data)
		return -ENOMEM;

	for (p = debugfs_entries; p->name; p++) {
		stat_data = kzalloc(sizeof(*stat_data), GFP_KERNEL_ACCOUNT);
		if (!stat_data)
			return -ENOMEM;

		stat_data->kvm = kvm;
		stat_data->offset = p->offset;
		kvm->debugfs_stat_data[p - debugfs_entries] = stat_data;
		debugfs_create_file(p->name, 0644, kvm->debugfs_dentry,
				    stat_data, stat_fops_per_vm[p->kind]);
	}
	return 0;
}
