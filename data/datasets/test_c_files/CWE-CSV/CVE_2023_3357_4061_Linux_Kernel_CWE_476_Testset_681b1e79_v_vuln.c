int amd_sfh_hid_client_init(struct amd_mp2_dev *privdata)
{
	struct amd_input_data *in_data = &privdata->in_data;
	struct amdtp_cl_data *cl_data = privdata->cl_data;
	struct amd_mp2_ops *mp2_ops = privdata->mp2_ops;
	struct amd_mp2_sensor_info info;
	struct request_list *req_list;
	struct device *dev;
	u32 feature_report_size;
	u32 input_report_size;
	int rc, i, status;
	u8 cl_idx;

	req_list = &cl_data->req_list;
	dev = &privdata->pdev->dev;
	amd_sfh_set_desc_ops(mp2_ops);

	mp2_ops->suspend = amd_sfh_suspend;
	mp2_ops->resume = amd_sfh_resume;

	cl_data->num_hid_devices = amd_mp2_get_sensor_num(privdata, &cl_data->sensor_idx[0]);
	if (cl_data->num_hid_devices == 0)
		return -ENODEV;

	INIT_DELAYED_WORK(&cl_data->work, amd_sfh_work);
	INIT_DELAYED_WORK(&cl_data->work_buffer, amd_sfh_work_buffer);
	INIT_LIST_HEAD(&req_list->list);
	cl_data->in_data = in_data;

	for (i = 0; i < cl_data->num_hid_devices; i++) {
		in_data->sensor_virt_addr[i] = dma_alloc_coherent(dev, sizeof(int) * 8,
								  &cl_data->sensor_dma_addr[i],
								  GFP_KERNEL);
		cl_data->sensor_sts[i] = SENSOR_DISABLED;
		cl_data->sensor_requested_cnt[i] = 0;
		cl_data->cur_hid_dev = i;
		cl_idx = cl_data->sensor_idx[i];
		cl_data->report_descr_sz[i] = mp2_ops->get_desc_sz(cl_idx, descr_size);
		if (!cl_data->report_descr_sz[i]) {
			rc = -EINVAL;
			goto cleanup;
		}
		feature_report_size = mp2_ops->get_desc_sz(cl_idx, feature_size);
		if (!feature_report_size) {
			rc = -EINVAL;
			goto cleanup;
		}
		input_report_size =  mp2_ops->get_desc_sz(cl_idx, input_size);
		if (!input_report_size) {
			rc = -EINVAL;
			goto cleanup;
		}
		cl_data->feature_report[i] = devm_kzalloc(dev, feature_report_size, GFP_KERNEL);
		if (!cl_data->feature_report[i]) {
			rc = -ENOMEM;
			goto cleanup;
		}
		in_data->input_report[i] = devm_kzalloc(dev, input_report_size, GFP_KERNEL);
		if (!in_data->input_report[i]) {
			rc = -ENOMEM;
			goto cleanup;
		}
		info.period = AMD_SFH_IDLE_LOOP;
		info.sensor_idx = cl_idx;
		info.dma_address = cl_data->sensor_dma_addr[i];

		cl_data->report_descr[i] =
			devm_kzalloc(dev, cl_data->report_descr_sz[i], GFP_KERNEL);
		if (!cl_data->report_descr[i]) {
			rc = -ENOMEM;
			goto cleanup;
		}
		rc = mp2_ops->get_rep_desc(cl_idx, cl_data->report_descr[i]);
		if (rc)
			return rc;
		mp2_ops->start(privdata, info);
		status = amd_sfh_wait_for_response
				(privdata, cl_data->sensor_idx[i], SENSOR_ENABLED);
		if (status == SENSOR_ENABLED) {
			cl_data->sensor_sts[i] = SENSOR_ENABLED;
			rc = amdtp_hid_probe(cl_data->cur_hid_dev, cl_data);
			if (rc) {
				mp2_ops->stop(privdata, cl_data->sensor_idx[i]);
				status = amd_sfh_wait_for_response
					(privdata, cl_data->sensor_idx[i], SENSOR_DISABLED);
				if (status != SENSOR_ENABLED)
					cl_data->sensor_sts[i] = SENSOR_DISABLED;
				dev_dbg(dev, "sid 0x%x (%s) status 0x%x\n",
					cl_data->sensor_idx[i],
					get_sensor_name(cl_data->sensor_idx[i]),
					cl_data->sensor_sts[i]);
				goto cleanup;
			}
		}
		dev_dbg(dev, "sid 0x%x (%s) status 0x%x\n",
			cl_data->sensor_idx[i], get_sensor_name(cl_data->sensor_idx[i]),
			cl_data->sensor_sts[i]);
	}
	if (mp2_ops->discovery_status && mp2_ops->discovery_status(privdata) == 0) {
		amd_sfh_hid_client_deinit(privdata);
		for (i = 0; i < cl_data->num_hid_devices; i++) {
			devm_kfree(dev, cl_data->feature_report[i]);
			devm_kfree(dev, in_data->input_report[i]);
			devm_kfree(dev, cl_data->report_descr[i]);
		}
		dev_warn(dev, "Failed to discover, sensors not enabled\n");
		return -EOPNOTSUPP;
	}
	schedule_delayed_work(&cl_data->work_buffer, msecs_to_jiffies(AMD_SFH_IDLE_LOOP));
	return 0;

cleanup:
	for (i = 0; i < cl_data->num_hid_devices; i++) {
		if (in_data->sensor_virt_addr[i]) {
			dma_free_coherent(&privdata->pdev->dev, 8 * sizeof(int),
					  in_data->sensor_virt_addr[i],
					  cl_data->sensor_dma_addr[i]);
		}
		devm_kfree(dev, cl_data->feature_report[i]);
		devm_kfree(dev, in_data->input_report[i]);
		devm_kfree(dev, cl_data->report_descr[i]);
	}
	return rc;
}
