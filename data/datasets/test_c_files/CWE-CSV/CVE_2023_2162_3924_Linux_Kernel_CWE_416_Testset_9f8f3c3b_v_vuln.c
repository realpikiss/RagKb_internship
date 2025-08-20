static struct iscsi_cls_session *
iscsi_sw_tcp_session_create(struct iscsi_endpoint *ep, uint16_t cmds_max,
			    uint16_t qdepth, uint32_t initial_cmdsn)
{
	struct iscsi_cls_session *cls_session;
	struct iscsi_session *session;
	struct iscsi_sw_tcp_host *tcp_sw_host;
	struct Scsi_Host *shost;
	int rc;

	if (ep) {
		printk(KERN_ERR "iscsi_tcp: invalid ep %p.\n", ep);
		return NULL;
	}

	shost = iscsi_host_alloc(&iscsi_sw_tcp_sht,
				 sizeof(struct iscsi_sw_tcp_host), 1);
	if (!shost)
		return NULL;
	shost->transportt = iscsi_sw_tcp_scsi_transport;
	shost->cmd_per_lun = qdepth;
	shost->max_lun = iscsi_max_lun;
	shost->max_id = 0;
	shost->max_channel = 0;
	shost->max_cmd_len = SCSI_MAX_VARLEN_CDB_SIZE;

	rc = iscsi_host_get_max_scsi_cmds(shost, cmds_max);
	if (rc < 0)
		goto free_host;
	shost->can_queue = rc;

	if (iscsi_host_add(shost, NULL))
		goto free_host;

	cls_session = iscsi_session_setup(&iscsi_sw_tcp_transport, shost,
					  cmds_max, 0,
					  sizeof(struct iscsi_tcp_task) +
					  sizeof(struct iscsi_sw_tcp_hdrbuf),
					  initial_cmdsn, 0);
	if (!cls_session)
		goto remove_host;
	session = cls_session->dd_data;
	tcp_sw_host = iscsi_host_priv(shost);
	tcp_sw_host->session = session;

	if (iscsi_tcp_r2tpool_alloc(session))
		goto remove_session;
	return cls_session;

remove_session:
	iscsi_session_teardown(cls_session);
remove_host:
	iscsi_host_remove(shost, false);
free_host:
	iscsi_host_free(shost);
	return NULL;
}
