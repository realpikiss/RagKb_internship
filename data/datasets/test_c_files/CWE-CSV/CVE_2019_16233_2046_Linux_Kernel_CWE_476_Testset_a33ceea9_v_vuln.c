static int
qla2x00_probe_one(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int	ret = -ENODEV;
	struct Scsi_Host *host;
	scsi_qla_host_t *base_vha = NULL;
	struct qla_hw_data *ha;
	char pci_info[30];
	char fw_str[30], wq_name[30];
	struct scsi_host_template *sht;
	int bars, mem_only = 0;
	uint16_t req_length = 0, rsp_length = 0;
	struct req_que *req = NULL;
	struct rsp_que *rsp = NULL;
	int i;

	bars = pci_select_bars(pdev, IORESOURCE_MEM | IORESOURCE_IO);
	sht = &qla2xxx_driver_template;
	if (pdev->device == PCI_DEVICE_ID_QLOGIC_ISP2422 ||
	    pdev->device == PCI_DEVICE_ID_QLOGIC_ISP2432 ||
	    pdev->device == PCI_DEVICE_ID_QLOGIC_ISP8432 ||
	    pdev->device == PCI_DEVICE_ID_QLOGIC_ISP5422 ||
	    pdev->device == PCI_DEVICE_ID_QLOGIC_ISP5432 ||
	    pdev->device == PCI_DEVICE_ID_QLOGIC_ISP2532 ||
	    pdev->device == PCI_DEVICE_ID_QLOGIC_ISP8001 ||
	    pdev->device == PCI_DEVICE_ID_QLOGIC_ISP8021 ||
	    pdev->device == PCI_DEVICE_ID_QLOGIC_ISP2031 ||
	    pdev->device == PCI_DEVICE_ID_QLOGIC_ISP8031 ||
	    pdev->device == PCI_DEVICE_ID_QLOGIC_ISPF001 ||
	    pdev->device == PCI_DEVICE_ID_QLOGIC_ISP8044 ||
	    pdev->device == PCI_DEVICE_ID_QLOGIC_ISP2071 ||
	    pdev->device == PCI_DEVICE_ID_QLOGIC_ISP2271 ||
	    pdev->device == PCI_DEVICE_ID_QLOGIC_ISP2261 ||
	    pdev->device == PCI_DEVICE_ID_QLOGIC_ISP2081 ||
	    pdev->device == PCI_DEVICE_ID_QLOGIC_ISP2281 ||
	    pdev->device == PCI_DEVICE_ID_QLOGIC_ISP2089 ||
	    pdev->device == PCI_DEVICE_ID_QLOGIC_ISP2289) {
		bars = pci_select_bars(pdev, IORESOURCE_MEM);
		mem_only = 1;
		ql_dbg_pci(ql_dbg_init, pdev, 0x0007,
		    "Mem only adapter.\n");
	}
	ql_dbg_pci(ql_dbg_init, pdev, 0x0008,
	    "Bars=%d.\n", bars);

	if (mem_only) {
		if (pci_enable_device_mem(pdev))
			return ret;
	} else {
		if (pci_enable_device(pdev))
			return ret;
	}

	/* This may fail but that's ok */
	pci_enable_pcie_error_reporting(pdev);

	/* Turn off T10-DIF when FC-NVMe is enabled */
	if (ql2xnvmeenable)
		ql2xenabledif = 0;

	ha = kzalloc(sizeof(struct qla_hw_data), GFP_KERNEL);
	if (!ha) {
		ql_log_pci(ql_log_fatal, pdev, 0x0009,
		    "Unable to allocate memory for ha.\n");
		goto disable_device;
	}
	ql_dbg_pci(ql_dbg_init, pdev, 0x000a,
	    "Memory allocated for ha=%p.\n", ha);
	ha->pdev = pdev;
	INIT_LIST_HEAD(&ha->tgt.q_full_list);
	spin_lock_init(&ha->tgt.q_full_lock);
	spin_lock_init(&ha->tgt.sess_lock);
	spin_lock_init(&ha->tgt.atio_lock);

	atomic_set(&ha->nvme_active_aen_cnt, 0);

	/* Clear our data area */
	ha->bars = bars;
	ha->mem_only = mem_only;
	spin_lock_init(&ha->hardware_lock);
	spin_lock_init(&ha->vport_slock);
	mutex_init(&ha->selflogin_lock);
	mutex_init(&ha->optrom_mutex);

	/* Set ISP-type information. */
	qla2x00_set_isp_flags(ha);

	/* Set EEH reset type to fundamental if required by hba */
	if (IS_QLA24XX(ha) || IS_QLA25XX(ha) || IS_QLA81XX(ha) ||
	    IS_QLA83XX(ha) || IS_QLA27XX(ha) || IS_QLA28XX(ha))
		pdev->needs_freset = 1;

	ha->prev_topology = 0;
	ha->init_cb_size = sizeof(init_cb_t);
	ha->link_data_rate = PORT_SPEED_UNKNOWN;
	ha->optrom_size = OPTROM_SIZE_2300;
	ha->max_exchg = FW_MAX_EXCHANGES_CNT;
	atomic_set(&ha->num_pend_mbx_stage1, 0);
	atomic_set(&ha->num_pend_mbx_stage2, 0);
	atomic_set(&ha->num_pend_mbx_stage3, 0);
	atomic_set(&ha->zio_threshold, DEFAULT_ZIO_THRESHOLD);
	ha->last_zio_threshold = DEFAULT_ZIO_THRESHOLD;

	/* Assign ISP specific operations. */
	if (IS_QLA2100(ha)) {
		ha->max_fibre_devices = MAX_FIBRE_DEVICES_2100;
		ha->mbx_count = MAILBOX_REGISTER_COUNT_2100;
		req_length = REQUEST_ENTRY_CNT_2100;
		rsp_length = RESPONSE_ENTRY_CNT_2100;
		ha->max_loop_id = SNS_LAST_LOOP_ID_2100;
		ha->gid_list_info_size = 4;
		ha->flash_conf_off = ~0;
		ha->flash_data_off = ~0;
		ha->nvram_conf_off = ~0;
		ha->nvram_data_off = ~0;
		ha->isp_ops = &qla2100_isp_ops;
	} else if (IS_QLA2200(ha)) {
		ha->max_fibre_devices = MAX_FIBRE_DEVICES_2100;
		ha->mbx_count = MAILBOX_REGISTER_COUNT_2200;
		req_length = REQUEST_ENTRY_CNT_2200;
		rsp_length = RESPONSE_ENTRY_CNT_2100;
		ha->max_loop_id = SNS_LAST_LOOP_ID_2100;
		ha->gid_list_info_size = 4;
		ha->flash_conf_off = ~0;
		ha->flash_data_off = ~0;
		ha->nvram_conf_off = ~0;
		ha->nvram_data_off = ~0;
		ha->isp_ops = &qla2100_isp_ops;
	} else if (IS_QLA23XX(ha)) {
		ha->max_fibre_devices = MAX_FIBRE_DEVICES_2100;
		ha->mbx_count = MAILBOX_REGISTER_COUNT;
		req_length = REQUEST_ENTRY_CNT_2200;
		rsp_length = RESPONSE_ENTRY_CNT_2300;
		ha->max_loop_id = SNS_LAST_LOOP_ID_2300;
		ha->gid_list_info_size = 6;
		if (IS_QLA2322(ha) || IS_QLA6322(ha))
			ha->optrom_size = OPTROM_SIZE_2322;
		ha->flash_conf_off = ~0;
		ha->flash_data_off = ~0;
		ha->nvram_conf_off = ~0;
		ha->nvram_data_off = ~0;
		ha->isp_ops = &qla2300_isp_ops;
	} else if (IS_QLA24XX_TYPE(ha)) {
		ha->max_fibre_devices = MAX_FIBRE_DEVICES_2400;
		ha->mbx_count = MAILBOX_REGISTER_COUNT;
		req_length = REQUEST_ENTRY_CNT_24XX;
		rsp_length = RESPONSE_ENTRY_CNT_2300;
		ha->tgt.atio_q_length = ATIO_ENTRY_CNT_24XX;
		ha->max_loop_id = SNS_LAST_LOOP_ID_2300;
		ha->init_cb_size = sizeof(struct mid_init_cb_24xx);
		ha->gid_list_info_size = 8;
		ha->optrom_size = OPTROM_SIZE_24XX;
		ha->nvram_npiv_size = QLA_MAX_VPORTS_QLA24XX;
		ha->isp_ops = &qla24xx_isp_ops;
		ha->flash_conf_off = FARX_ACCESS_FLASH_CONF;
		ha->flash_data_off = FARX_ACCESS_FLASH_DATA;
		ha->nvram_conf_off = FARX_ACCESS_NVRAM_CONF;
		ha->nvram_data_off = FARX_ACCESS_NVRAM_DATA;
	} else if (IS_QLA25XX(ha)) {
		ha->max_fibre_devices = MAX_FIBRE_DEVICES_2400;
		ha->mbx_count = MAILBOX_REGISTER_COUNT;
		req_length = REQUEST_ENTRY_CNT_24XX;
		rsp_length = RESPONSE_ENTRY_CNT_2300;
		ha->tgt.atio_q_length = ATIO_ENTRY_CNT_24XX;
		ha->max_loop_id = SNS_LAST_LOOP_ID_2300;
		ha->init_cb_size = sizeof(struct mid_init_cb_24xx);
		ha->gid_list_info_size = 8;
		ha->optrom_size = OPTROM_SIZE_25XX;
		ha->nvram_npiv_size = QLA_MAX_VPORTS_QLA25XX;
		ha->isp_ops = &qla25xx_isp_ops;
		ha->flash_conf_off = FARX_ACCESS_FLASH_CONF;
		ha->flash_data_off = FARX_ACCESS_FLASH_DATA;
		ha->nvram_conf_off = FARX_ACCESS_NVRAM_CONF;
		ha->nvram_data_off = FARX_ACCESS_NVRAM_DATA;
	} else if (IS_QLA81XX(ha)) {
		ha->max_fibre_devices = MAX_FIBRE_DEVICES_2400;
		ha->mbx_count = MAILBOX_REGISTER_COUNT;
		req_length = REQUEST_ENTRY_CNT_24XX;
		rsp_length = RESPONSE_ENTRY_CNT_2300;
		ha->tgt.atio_q_length = ATIO_ENTRY_CNT_24XX;
		ha->max_loop_id = SNS_LAST_LOOP_ID_2300;
		ha->init_cb_size = sizeof(struct mid_init_cb_81xx);
		ha->gid_list_info_size = 8;
		ha->optrom_size = OPTROM_SIZE_81XX;
		ha->nvram_npiv_size = QLA_MAX_VPORTS_QLA25XX;
		ha->isp_ops = &qla81xx_isp_ops;
		ha->flash_conf_off = FARX_ACCESS_FLASH_CONF_81XX;
		ha->flash_data_off = FARX_ACCESS_FLASH_DATA_81XX;
		ha->nvram_conf_off = ~0;
		ha->nvram_data_off = ~0;
	} else if (IS_QLA82XX(ha)) {
		ha->max_fibre_devices = MAX_FIBRE_DEVICES_2400;
		ha->mbx_count = MAILBOX_REGISTER_COUNT;
		req_length = REQUEST_ENTRY_CNT_82XX;
		rsp_length = RESPONSE_ENTRY_CNT_82XX;
		ha->max_loop_id = SNS_LAST_LOOP_ID_2300;
		ha->init_cb_size = sizeof(struct mid_init_cb_81xx);
		ha->gid_list_info_size = 8;
		ha->optrom_size = OPTROM_SIZE_82XX;
		ha->nvram_npiv_size = QLA_MAX_VPORTS_QLA25XX;
		ha->isp_ops = &qla82xx_isp_ops;
		ha->flash_conf_off = FARX_ACCESS_FLASH_CONF;
		ha->flash_data_off = FARX_ACCESS_FLASH_DATA;
		ha->nvram_conf_off = FARX_ACCESS_NVRAM_CONF;
		ha->nvram_data_off = FARX_ACCESS_NVRAM_DATA;
	} else if (IS_QLA8044(ha)) {
		ha->max_fibre_devices = MAX_FIBRE_DEVICES_2400;
		ha->mbx_count = MAILBOX_REGISTER_COUNT;
		req_length = REQUEST_ENTRY_CNT_82XX;
		rsp_length = RESPONSE_ENTRY_CNT_82XX;
		ha->max_loop_id = SNS_LAST_LOOP_ID_2300;
		ha->init_cb_size = sizeof(struct mid_init_cb_81xx);
		ha->gid_list_info_size = 8;
		ha->optrom_size = OPTROM_SIZE_83XX;
		ha->nvram_npiv_size = QLA_MAX_VPORTS_QLA25XX;
		ha->isp_ops = &qla8044_isp_ops;
		ha->flash_conf_off = FARX_ACCESS_FLASH_CONF;
		ha->flash_data_off = FARX_ACCESS_FLASH_DATA;
		ha->nvram_conf_off = FARX_ACCESS_NVRAM_CONF;
		ha->nvram_data_off = FARX_ACCESS_NVRAM_DATA;
	} else if (IS_QLA83XX(ha)) {
		ha->portnum = PCI_FUNC(ha->pdev->devfn);
		ha->max_fibre_devices = MAX_FIBRE_DEVICES_2400;
		ha->mbx_count = MAILBOX_REGISTER_COUNT;
		req_length = REQUEST_ENTRY_CNT_83XX;
		rsp_length = RESPONSE_ENTRY_CNT_83XX;
		ha->tgt.atio_q_length = ATIO_ENTRY_CNT_24XX;
		ha->max_loop_id = SNS_LAST_LOOP_ID_2300;
		ha->init_cb_size = sizeof(struct mid_init_cb_81xx);
		ha->gid_list_info_size = 8;
		ha->optrom_size = OPTROM_SIZE_83XX;
		ha->nvram_npiv_size = QLA_MAX_VPORTS_QLA25XX;
		ha->isp_ops = &qla83xx_isp_ops;
		ha->flash_conf_off = FARX_ACCESS_FLASH_CONF_81XX;
		ha->flash_data_off = FARX_ACCESS_FLASH_DATA_81XX;
		ha->nvram_conf_off = ~0;
		ha->nvram_data_off = ~0;
	}  else if (IS_QLAFX00(ha)) {
		ha->max_fibre_devices = MAX_FIBRE_DEVICES_FX00;
		ha->mbx_count = MAILBOX_REGISTER_COUNT_FX00;
		ha->aen_mbx_count = AEN_MAILBOX_REGISTER_COUNT_FX00;
		req_length = REQUEST_ENTRY_CNT_FX00;
		rsp_length = RESPONSE_ENTRY_CNT_FX00;
		ha->isp_ops = &qlafx00_isp_ops;
		ha->port_down_retry_count = 30; /* default value */
		ha->mr.fw_hbt_cnt = QLAFX00_HEARTBEAT_INTERVAL;
		ha->mr.fw_reset_timer_tick = QLAFX00_RESET_INTERVAL;
		ha->mr.fw_critemp_timer_tick = QLAFX00_CRITEMP_INTERVAL;
		ha->mr.fw_hbt_en = 1;
		ha->mr.host_info_resend = false;
		ha->mr.hinfo_resend_timer_tick = QLAFX00_HINFO_RESEND_INTERVAL;
	} else if (IS_QLA27XX(ha)) {
		ha->portnum = PCI_FUNC(ha->pdev->devfn);
		ha->max_fibre_devices = MAX_FIBRE_DEVICES_2400;
		ha->mbx_count = MAILBOX_REGISTER_COUNT;
		req_length = REQUEST_ENTRY_CNT_83XX;
		rsp_length = RESPONSE_ENTRY_CNT_83XX;
		ha->tgt.atio_q_length = ATIO_ENTRY_CNT_24XX;
		ha->max_loop_id = SNS_LAST_LOOP_ID_2300;
		ha->init_cb_size = sizeof(struct mid_init_cb_81xx);
		ha->gid_list_info_size = 8;
		ha->optrom_size = OPTROM_SIZE_83XX;
		ha->nvram_npiv_size = QLA_MAX_VPORTS_QLA25XX;
		ha->isp_ops = &qla27xx_isp_ops;
		ha->flash_conf_off = FARX_ACCESS_FLASH_CONF_81XX;
		ha->flash_data_off = FARX_ACCESS_FLASH_DATA_81XX;
		ha->nvram_conf_off = ~0;
		ha->nvram_data_off = ~0;
	} else if (IS_QLA28XX(ha)) {
		ha->portnum = PCI_FUNC(ha->pdev->devfn);
		ha->max_fibre_devices = MAX_FIBRE_DEVICES_2400;
		ha->mbx_count = MAILBOX_REGISTER_COUNT;
		req_length = REQUEST_ENTRY_CNT_24XX;
		rsp_length = RESPONSE_ENTRY_CNT_2300;
		ha->tgt.atio_q_length = ATIO_ENTRY_CNT_24XX;
		ha->max_loop_id = SNS_LAST_LOOP_ID_2300;
		ha->init_cb_size = sizeof(struct mid_init_cb_81xx);
		ha->gid_list_info_size = 8;
		ha->optrom_size = OPTROM_SIZE_28XX;
		ha->nvram_npiv_size = QLA_MAX_VPORTS_QLA25XX;
		ha->isp_ops = &qla27xx_isp_ops;
		ha->flash_conf_off = FARX_ACCESS_FLASH_CONF_28XX;
		ha->flash_data_off = FARX_ACCESS_FLASH_DATA_28XX;
		ha->nvram_conf_off = ~0;
		ha->nvram_data_off = ~0;
	}

	ql_dbg_pci(ql_dbg_init, pdev, 0x001e,
	    "mbx_count=%d, req_length=%d, "
	    "rsp_length=%d, max_loop_id=%d, init_cb_size=%d, "
	    "gid_list_info_size=%d, optrom_size=%d, nvram_npiv_size=%d, "
	    "max_fibre_devices=%d.\n",
	    ha->mbx_count, req_length, rsp_length, ha->max_loop_id,
	    ha->init_cb_size, ha->gid_list_info_size, ha->optrom_size,
	    ha->nvram_npiv_size, ha->max_fibre_devices);
	ql_dbg_pci(ql_dbg_init, pdev, 0x001f,
	    "isp_ops=%p, flash_conf_off=%d, "
	    "flash_data_off=%d, nvram_conf_off=%d, nvram_data_off=%d.\n",
	    ha->isp_ops, ha->flash_conf_off, ha->flash_data_off,
	    ha->nvram_conf_off, ha->nvram_data_off);

	/* Configure PCI I/O space */
	ret = ha->isp_ops->iospace_config(ha);
	if (ret)
		goto iospace_config_failed;

	ql_log_pci(ql_log_info, pdev, 0x001d,
	    "Found an ISP%04X irq %d iobase 0x%p.\n",
	    pdev->device, pdev->irq, ha->iobase);
	mutex_init(&ha->vport_lock);
	mutex_init(&ha->mq_lock);
	init_completion(&ha->mbx_cmd_comp);
	complete(&ha->mbx_cmd_comp);
	init_completion(&ha->mbx_intr_comp);
	init_completion(&ha->dcbx_comp);
	init_completion(&ha->lb_portup_comp);

	set_bit(0, (unsigned long *) ha->vp_idx_map);

	qla2x00_config_dma_addressing(ha);
	ql_dbg_pci(ql_dbg_init, pdev, 0x0020,
	    "64 Bit addressing is %s.\n",
	    ha->flags.enable_64bit_addressing ? "enable" :
	    "disable");
	ret = qla2x00_mem_alloc(ha, req_length, rsp_length, &req, &rsp);
	if (ret) {
		ql_log_pci(ql_log_fatal, pdev, 0x0031,
		    "Failed to allocate memory for adapter, aborting.\n");

		goto probe_hw_failed;
	}

	req->max_q_depth = MAX_Q_DEPTH;
	if (ql2xmaxqdepth != 0 && ql2xmaxqdepth <= 0xffffU)
		req->max_q_depth = ql2xmaxqdepth;


	base_vha = qla2x00_create_host(sht, ha);
	if (!base_vha) {
		ret = -ENOMEM;
		goto probe_hw_failed;
	}

	pci_set_drvdata(pdev, base_vha);
	set_bit(PFLG_DRIVER_PROBING, &base_vha->pci_flags);

	host = base_vha->host;
	base_vha->req = req;
	if (IS_QLA2XXX_MIDTYPE(ha))
		base_vha->mgmt_svr_loop_id =
			qla2x00_reserve_mgmt_server_loop_id(base_vha);
	else
		base_vha->mgmt_svr_loop_id = MANAGEMENT_SERVER +
						base_vha->vp_idx;

	/* Setup fcport template structure. */
	ha->mr.fcport.vha = base_vha;
	ha->mr.fcport.port_type = FCT_UNKNOWN;
	ha->mr.fcport.loop_id = FC_NO_LOOP_ID;
	qla2x00_set_fcport_state(&ha->mr.fcport, FCS_UNCONFIGURED);
	ha->mr.fcport.supported_classes = FC_COS_UNSPECIFIED;
	ha->mr.fcport.scan_state = 1;

	/* Set the SG table size based on ISP type */
	if (!IS_FWI2_CAPABLE(ha)) {
		if (IS_QLA2100(ha))
			host->sg_tablesize = 32;
	} else {
		if (!IS_QLA82XX(ha))
			host->sg_tablesize = QLA_SG_ALL;
	}
	host->max_id = ha->max_fibre_devices;
	host->cmd_per_lun = 3;
	host->unique_id = host->host_no;
	if (IS_T10_PI_CAPABLE(ha) && ql2xenabledif)
		host->max_cmd_len = 32;
	else
		host->max_cmd_len = MAX_CMDSZ;
	host->max_channel = MAX_BUSES - 1;
	/* Older HBAs support only 16-bit LUNs */
	if (!IS_QLAFX00(ha) && !IS_FWI2_CAPABLE(ha) &&
	    ql2xmaxlun > 0xffff)
		host->max_lun = 0xffff;
	else
		host->max_lun = ql2xmaxlun;
	host->transportt = qla2xxx_transport_template;
	sht->vendor_id = (SCSI_NL_VID_TYPE_PCI | PCI_VENDOR_ID_QLOGIC);

	ql_dbg(ql_dbg_init, base_vha, 0x0033,
	    "max_id=%d this_id=%d "
	    "cmd_per_len=%d unique_id=%d max_cmd_len=%d max_channel=%d "
	    "max_lun=%llu transportt=%p, vendor_id=%llu.\n", host->max_id,
	    host->this_id, host->cmd_per_lun, host->unique_id,
	    host->max_cmd_len, host->max_channel, host->max_lun,
	    host->transportt, sht->vendor_id);

	INIT_WORK(&base_vha->iocb_work, qla2x00_iocb_work_fn);

	/* Set up the irqs */
	ret = qla2x00_request_irqs(ha, rsp);
	if (ret)
		goto probe_failed;

	/* Alloc arrays of request and response ring ptrs */
	ret = qla2x00_alloc_queues(ha, req, rsp);
	if (ret) {
		ql_log(ql_log_fatal, base_vha, 0x003d,
		    "Failed to allocate memory for queue pointers..."
		    "aborting.\n");
		ret = -ENODEV;
		goto probe_failed;
	}

	if (ha->mqenable) {
		/* number of hardware queues supported by blk/scsi-mq*/
		host->nr_hw_queues = ha->max_qpairs;

		ql_dbg(ql_dbg_init, base_vha, 0x0192,
			"blk/scsi-mq enabled, HW queues = %d.\n", host->nr_hw_queues);
	} else {
		if (ql2xnvmeenable) {
			host->nr_hw_queues = ha->max_qpairs;
			ql_dbg(ql_dbg_init, base_vha, 0x0194,
			    "FC-NVMe support is enabled, HW queues=%d\n",
			    host->nr_hw_queues);
		} else {
			ql_dbg(ql_dbg_init, base_vha, 0x0193,
			    "blk/scsi-mq disabled.\n");
		}
	}

	qlt_probe_one_stage1(base_vha, ha);

	pci_save_state(pdev);

	/* Assign back pointers */
	rsp->req = req;
	req->rsp = rsp;

	if (IS_QLAFX00(ha)) {
		ha->rsp_q_map[0] = rsp;
		ha->req_q_map[0] = req;
		set_bit(0, ha->req_qid_map);
		set_bit(0, ha->rsp_qid_map);
	}

	/* FWI2-capable only. */
	req->req_q_in = &ha->iobase->isp24.req_q_in;
	req->req_q_out = &ha->iobase->isp24.req_q_out;
	rsp->rsp_q_in = &ha->iobase->isp24.rsp_q_in;
	rsp->rsp_q_out = &ha->iobase->isp24.rsp_q_out;
	if (ha->mqenable || IS_QLA83XX(ha) || IS_QLA27XX(ha) ||
	    IS_QLA28XX(ha)) {
		req->req_q_in = &ha->mqiobase->isp25mq.req_q_in;
		req->req_q_out = &ha->mqiobase->isp25mq.req_q_out;
		rsp->rsp_q_in = &ha->mqiobase->isp25mq.rsp_q_in;
		rsp->rsp_q_out =  &ha->mqiobase->isp25mq.rsp_q_out;
	}

	if (IS_QLAFX00(ha)) {
		req->req_q_in = &ha->iobase->ispfx00.req_q_in;
		req->req_q_out = &ha->iobase->ispfx00.req_q_out;
		rsp->rsp_q_in = &ha->iobase->ispfx00.rsp_q_in;
		rsp->rsp_q_out = &ha->iobase->ispfx00.rsp_q_out;
	}

	if (IS_P3P_TYPE(ha)) {
		req->req_q_out = &ha->iobase->isp82.req_q_out[0];
		rsp->rsp_q_in = &ha->iobase->isp82.rsp_q_in[0];
		rsp->rsp_q_out = &ha->iobase->isp82.rsp_q_out[0];
	}

	ql_dbg(ql_dbg_multiq, base_vha, 0xc009,
	    "rsp_q_map=%p req_q_map=%p rsp->req=%p req->rsp=%p.\n",
	    ha->rsp_q_map, ha->req_q_map, rsp->req, req->rsp);
	ql_dbg(ql_dbg_multiq, base_vha, 0xc00a,
	    "req->req_q_in=%p req->req_q_out=%p "
	    "rsp->rsp_q_in=%p rsp->rsp_q_out=%p.\n",
	    req->req_q_in, req->req_q_out,
	    rsp->rsp_q_in, rsp->rsp_q_out);
	ql_dbg(ql_dbg_init, base_vha, 0x003e,
	    "rsp_q_map=%p req_q_map=%p rsp->req=%p req->rsp=%p.\n",
	    ha->rsp_q_map, ha->req_q_map, rsp->req, req->rsp);
	ql_dbg(ql_dbg_init, base_vha, 0x003f,
	    "req->req_q_in=%p req->req_q_out=%p rsp->rsp_q_in=%p rsp->rsp_q_out=%p.\n",
	    req->req_q_in, req->req_q_out, rsp->rsp_q_in, rsp->rsp_q_out);

	ha->wq = alloc_workqueue("qla2xxx_wq", 0, 0);

	if (ha->isp_ops->initialize_adapter(base_vha)) {
		ql_log(ql_log_fatal, base_vha, 0x00d6,
		    "Failed to initialize adapter - Adapter flags %x.\n",
		    base_vha->device_flags);

		if (IS_QLA82XX(ha)) {
			qla82xx_idc_lock(ha);
			qla82xx_wr_32(ha, QLA82XX_CRB_DEV_STATE,
				QLA8XXX_DEV_FAILED);
			qla82xx_idc_unlock(ha);
			ql_log(ql_log_fatal, base_vha, 0x00d7,
			    "HW State: FAILED.\n");
		} else if (IS_QLA8044(ha)) {
			qla8044_idc_lock(ha);
			qla8044_wr_direct(base_vha,
				QLA8044_CRB_DEV_STATE_INDEX,
				QLA8XXX_DEV_FAILED);
			qla8044_idc_unlock(ha);
			ql_log(ql_log_fatal, base_vha, 0x0150,
			    "HW State: FAILED.\n");
		}

		ret = -ENODEV;
		goto probe_failed;
	}

	if (IS_QLAFX00(ha))
		host->can_queue = QLAFX00_MAX_CANQUEUE;
	else
		host->can_queue = req->num_outstanding_cmds - 10;

	ql_dbg(ql_dbg_init, base_vha, 0x0032,
	    "can_queue=%d, req=%p, mgmt_svr_loop_id=%d, sg_tablesize=%d.\n",
	    host->can_queue, base_vha->req,
	    base_vha->mgmt_svr_loop_id, host->sg_tablesize);

	if (ha->mqenable) {
		bool startit = false;

		if (QLA_TGT_MODE_ENABLED())
			startit = false;

		if (ql2x_ini_mode == QLA2XXX_INI_MODE_ENABLED)
			startit = true;

		/* Create start of day qpairs for Block MQ */
		for (i = 0; i < ha->max_qpairs; i++)
			qla2xxx_create_qpair(base_vha, 5, 0, startit);
	}

	if (ha->flags.running_gold_fw)
		goto skip_dpc;

	/*
	 * Startup the kernel thread for this host adapter
	 */
	ha->dpc_thread = kthread_create(qla2x00_do_dpc, ha,
	    "%s_dpc", base_vha->host_str);
	if (IS_ERR(ha->dpc_thread)) {
		ql_log(ql_log_fatal, base_vha, 0x00ed,
		    "Failed to start DPC thread.\n");
		ret = PTR_ERR(ha->dpc_thread);
		ha->dpc_thread = NULL;
		goto probe_failed;
	}
	ql_dbg(ql_dbg_init, base_vha, 0x00ee,
	    "DPC thread started successfully.\n");

	/*
	 * If we're not coming up in initiator mode, we might sit for
	 * a while without waking up the dpc thread, which leads to a
	 * stuck process warning.  So just kick the dpc once here and
	 * let the kthread start (and go back to sleep in qla2x00_do_dpc).
	 */
	qla2xxx_wake_dpc(base_vha);

	INIT_WORK(&ha->board_disable, qla2x00_disable_board_on_pci_error);

	if (IS_QLA8031(ha) || IS_MCTP_CAPABLE(ha)) {
		sprintf(wq_name, "qla2xxx_%lu_dpc_lp_wq", base_vha->host_no);
		ha->dpc_lp_wq = create_singlethread_workqueue(wq_name);
		INIT_WORK(&ha->idc_aen, qla83xx_service_idc_aen);

		sprintf(wq_name, "qla2xxx_%lu_dpc_hp_wq", base_vha->host_no);
		ha->dpc_hp_wq = create_singlethread_workqueue(wq_name);
		INIT_WORK(&ha->nic_core_reset, qla83xx_nic_core_reset_work);
		INIT_WORK(&ha->idc_state_handler,
		    qla83xx_idc_state_handler_work);
		INIT_WORK(&ha->nic_core_unrecoverable,
		    qla83xx_nic_core_unrecoverable_work);
	}

skip_dpc:
	list_add_tail(&base_vha->list, &ha->vp_list);
	base_vha->host->irq = ha->pdev->irq;

	/* Initialized the timer */
	qla2x00_start_timer(base_vha, WATCH_INTERVAL);
	ql_dbg(ql_dbg_init, base_vha, 0x00ef,
	    "Started qla2x00_timer with "
	    "interval=%d.\n", WATCH_INTERVAL);
	ql_dbg(ql_dbg_init, base_vha, 0x00f0,
	    "Detected hba at address=%p.\n",
	    ha);

	if (IS_T10_PI_CAPABLE(ha) && ql2xenabledif) {
		if (ha->fw_attributes & BIT_4) {
			int prot = 0, guard;

			base_vha->flags.difdix_supported = 1;
			ql_dbg(ql_dbg_init, base_vha, 0x00f1,
			    "Registering for DIF/DIX type 1 and 3 protection.\n");
			if (ql2xenabledif == 1)
				prot = SHOST_DIX_TYPE0_PROTECTION;
			if (ql2xprotmask)
				scsi_host_set_prot(host, ql2xprotmask);
			else
				scsi_host_set_prot(host,
				    prot | SHOST_DIF_TYPE1_PROTECTION
				    | SHOST_DIF_TYPE2_PROTECTION
				    | SHOST_DIF_TYPE3_PROTECTION
				    | SHOST_DIX_TYPE1_PROTECTION
				    | SHOST_DIX_TYPE2_PROTECTION
				    | SHOST_DIX_TYPE3_PROTECTION);

			guard = SHOST_DIX_GUARD_CRC;

			if (IS_PI_IPGUARD_CAPABLE(ha) &&
			    (ql2xenabledif > 1 || IS_PI_DIFB_DIX0_CAPABLE(ha)))
				guard |= SHOST_DIX_GUARD_IP;

			if (ql2xprotguard)
				scsi_host_set_guard(host, ql2xprotguard);
			else
				scsi_host_set_guard(host, guard);
		} else
			base_vha->flags.difdix_supported = 0;
	}

	ha->isp_ops->enable_intrs(ha);

	if (IS_QLAFX00(ha)) {
		ret = qlafx00_fx_disc(base_vha,
			&base_vha->hw->mr.fcport, FXDISC_GET_CONFIG_INFO);
		host->sg_tablesize = (ha->mr.extended_io_enabled) ?
		    QLA_SG_ALL : 128;
	}

	ret = scsi_add_host(host, &pdev->dev);
	if (ret)
		goto probe_failed;

	base_vha->flags.init_done = 1;
	base_vha->flags.online = 1;
	ha->prev_minidump_failed = 0;

	ql_dbg(ql_dbg_init, base_vha, 0x00f2,
	    "Init done and hba is online.\n");

	if (qla_ini_mode_enabled(base_vha) ||
		qla_dual_mode_enabled(base_vha))
		scsi_scan_host(host);
	else
		ql_dbg(ql_dbg_init, base_vha, 0x0122,
			"skipping scsi_scan_host() for non-initiator port\n");

	qla2x00_alloc_sysfs_attr(base_vha);

	if (IS_QLAFX00(ha)) {
		ret = qlafx00_fx_disc(base_vha,
			&base_vha->hw->mr.fcport, FXDISC_GET_PORT_INFO);

		/* Register system information */
		ret =  qlafx00_fx_disc(base_vha,
			&base_vha->hw->mr.fcport, FXDISC_REG_HOST_INFO);
	}

	qla2x00_init_host_attr(base_vha);

	qla2x00_dfs_setup(base_vha);

	ql_log(ql_log_info, base_vha, 0x00fb,
	    "QLogic %s - %s.\n", ha->model_number, ha->model_desc);
	ql_log(ql_log_info, base_vha, 0x00fc,
	    "ISP%04X: %s @ %s hdma%c host#=%ld fw=%s.\n",
	    pdev->device, ha->isp_ops->pci_info_str(base_vha, pci_info,
						       sizeof(pci_info)),
	    pci_name(pdev), ha->flags.enable_64bit_addressing ? '+' : '-',
	    base_vha->host_no,
	    ha->isp_ops->fw_version_str(base_vha, fw_str, sizeof(fw_str)));

	qlt_add_target(ha, base_vha);

	clear_bit(PFLG_DRIVER_PROBING, &base_vha->pci_flags);

	if (test_bit(UNLOADING, &base_vha->dpc_flags))
		return -ENODEV;

	if (ha->flags.detected_lr_sfp) {
		ql_log(ql_log_info, base_vha, 0xffff,
		    "Reset chip to pick up LR SFP setting\n");
		set_bit(ISP_ABORT_NEEDED, &base_vha->dpc_flags);
		qla2xxx_wake_dpc(base_vha);
	}

	return 0;

probe_failed:
	if (base_vha->timer_active)
		qla2x00_stop_timer(base_vha);
	base_vha->flags.online = 0;
	if (ha->dpc_thread) {
		struct task_struct *t = ha->dpc_thread;

		ha->dpc_thread = NULL;
		kthread_stop(t);
	}

	qla2x00_free_device(base_vha);
	scsi_host_put(base_vha->host);
	/*
	 * Need to NULL out local req/rsp after
	 * qla2x00_free_device => qla2x00_free_queues frees
	 * what these are pointing to. Or else we'll
	 * fall over below in qla2x00_free_req/rsp_que.
	 */
	req = NULL;
	rsp = NULL;

probe_hw_failed:
	qla2x00_mem_free(ha);
	qla2x00_free_req_que(ha, req);
	qla2x00_free_rsp_que(ha, rsp);
	qla2x00_clear_drv_active(ha);

iospace_config_failed:
	if (IS_P3P_TYPE(ha)) {
		if (!ha->nx_pcibase)
			iounmap((device_reg_t *)ha->nx_pcibase);
		if (!ql2xdbwr)
			iounmap((device_reg_t *)ha->nxdb_wr_ptr);
	} else {
		if (ha->iobase)
			iounmap(ha->iobase);
		if (ha->cregbase)
			iounmap(ha->cregbase);
	}
	pci_release_selected_regions(ha->pdev, ha->bars);
	kfree(ha);

disable_device:
	pci_disable_device(pdev);
	return ret;
}
