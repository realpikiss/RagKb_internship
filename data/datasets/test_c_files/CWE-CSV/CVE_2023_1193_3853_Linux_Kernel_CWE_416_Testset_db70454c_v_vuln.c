int setup_async_work(struct ksmbd_work *work, void (*fn)(void **), void **arg)
{
	struct smb2_hdr *rsp_hdr;
	struct ksmbd_conn *conn = work->conn;
	int id;

	rsp_hdr = smb2_get_msg(work->response_buf);
	rsp_hdr->Flags |= SMB2_FLAGS_ASYNC_COMMAND;

	id = ksmbd_acquire_async_msg_id(&conn->async_ida);
	if (id < 0) {
		pr_err("Failed to alloc async message id\n");
		return id;
	}
	work->synchronous = false;
	work->async_id = id;
	rsp_hdr->Id.AsyncId = cpu_to_le64(id);

	ksmbd_debug(SMB,
		    "Send interim Response to inform async request id : %d\n",
		    work->async_id);

	work->cancel_fn = fn;
	work->cancel_argv = arg;

	if (list_empty(&work->async_request_entry)) {
		spin_lock(&conn->request_lock);
		list_add_tail(&work->async_request_entry, &conn->async_requests);
		spin_unlock(&conn->request_lock);
	}

	return 0;
}
