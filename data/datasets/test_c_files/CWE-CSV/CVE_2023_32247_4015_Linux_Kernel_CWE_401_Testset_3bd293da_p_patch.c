int smb2_sess_setup(struct ksmbd_work *work)
{
	struct ksmbd_conn *conn = work->conn;
	struct smb2_sess_setup_req *req = smb2_get_msg(work->request_buf);
	struct smb2_sess_setup_rsp *rsp = smb2_get_msg(work->response_buf);
	struct ksmbd_session *sess;
	struct negotiate_message *negblob;
	unsigned int negblob_len, negblob_off;
	int rc = 0;

	ksmbd_debug(SMB, "Received request for session setup\n");

	rsp->StructureSize = cpu_to_le16(9);
	rsp->SessionFlags = 0;
	rsp->SecurityBufferOffset = cpu_to_le16(72);
	rsp->SecurityBufferLength = 0;
	inc_rfc1001_len(work->response_buf, 9);

	ksmbd_conn_lock(conn);
	if (!req->hdr.SessionId) {
		sess = ksmbd_smb2_session_create();
		if (!sess) {
			rc = -ENOMEM;
			goto out_err;
		}
		rsp->hdr.SessionId = cpu_to_le64(sess->id);
		rc = ksmbd_session_register(conn, sess);
		if (rc)
			goto out_err;
	} else if (conn->dialect >= SMB30_PROT_ID &&
		   (server_conf.flags & KSMBD_GLOBAL_FLAG_SMB3_MULTICHANNEL) &&
		   req->Flags & SMB2_SESSION_REQ_FLAG_BINDING) {
		u64 sess_id = le64_to_cpu(req->hdr.SessionId);

		sess = ksmbd_session_lookup_slowpath(sess_id);
		if (!sess) {
			rc = -ENOENT;
			goto out_err;
		}

		if (conn->dialect != sess->dialect) {
			rc = -EINVAL;
			goto out_err;
		}

		if (!(req->hdr.Flags & SMB2_FLAGS_SIGNED)) {
			rc = -EINVAL;
			goto out_err;
		}

		if (strncmp(conn->ClientGUID, sess->ClientGUID,
			    SMB2_CLIENT_GUID_SIZE)) {
			rc = -ENOENT;
			goto out_err;
		}

		if (sess->state == SMB2_SESSION_IN_PROGRESS) {
			rc = -EACCES;
			goto out_err;
		}

		if (sess->state == SMB2_SESSION_EXPIRED) {
			rc = -EFAULT;
			goto out_err;
		}

		if (ksmbd_conn_need_reconnect(conn)) {
			rc = -EFAULT;
			sess = NULL;
			goto out_err;
		}

		if (ksmbd_session_lookup(conn, sess_id)) {
			rc = -EACCES;
			goto out_err;
		}

		conn->binding = true;
	} else if ((conn->dialect < SMB30_PROT_ID ||
		    server_conf.flags & KSMBD_GLOBAL_FLAG_SMB3_MULTICHANNEL) &&
		   (req->Flags & SMB2_SESSION_REQ_FLAG_BINDING)) {
		sess = NULL;
		rc = -EACCES;
		goto out_err;
	} else {
		sess = ksmbd_session_lookup(conn,
					    le64_to_cpu(req->hdr.SessionId));
		if (!sess) {
			rc = -ENOENT;
			goto out_err;
		}

		if (sess->state == SMB2_SESSION_EXPIRED) {
			rc = -EFAULT;
			goto out_err;
		}

		if (ksmbd_conn_need_reconnect(conn)) {
			rc = -EFAULT;
			sess = NULL;
			goto out_err;
		}
	}
	work->sess = sess;

	negblob_off = le16_to_cpu(req->SecurityBufferOffset);
	negblob_len = le16_to_cpu(req->SecurityBufferLength);
	if (negblob_off < offsetof(struct smb2_sess_setup_req, Buffer) ||
	    negblob_len < offsetof(struct negotiate_message, NegotiateFlags)) {
		rc = -EINVAL;
		goto out_err;
	}

	negblob = (struct negotiate_message *)((char *)&req->hdr.ProtocolId +
			negblob_off);

	if (decode_negotiation_token(conn, negblob, negblob_len) == 0) {
		if (conn->mechToken)
			negblob = (struct negotiate_message *)conn->mechToken;
	}

	if (server_conf.auth_mechs & conn->auth_mechs) {
		rc = generate_preauth_hash(work);
		if (rc)
			goto out_err;

		if (conn->preferred_auth_mech &
				(KSMBD_AUTH_KRB5 | KSMBD_AUTH_MSKRB5)) {
			rc = krb5_authenticate(work);
			if (rc) {
				rc = -EINVAL;
				goto out_err;
			}

			if (!ksmbd_conn_need_reconnect(conn)) {
				ksmbd_conn_set_good(conn);
				sess->state = SMB2_SESSION_VALID;
			}
			kfree(sess->Preauth_HashValue);
			sess->Preauth_HashValue = NULL;
		} else if (conn->preferred_auth_mech == KSMBD_AUTH_NTLMSSP) {
			if (negblob->MessageType == NtLmNegotiate) {
				rc = ntlm_negotiate(work, negblob, negblob_len);
				if (rc)
					goto out_err;
				rsp->hdr.Status =
					STATUS_MORE_PROCESSING_REQUIRED;
				/*
				 * Note: here total size -1 is done as an
				 * adjustment for 0 size blob
				 */
				inc_rfc1001_len(work->response_buf,
						le16_to_cpu(rsp->SecurityBufferLength) - 1);

			} else if (negblob->MessageType == NtLmAuthenticate) {
				rc = ntlm_authenticate(work);
				if (rc)
					goto out_err;

				if (!ksmbd_conn_need_reconnect(conn)) {
					ksmbd_conn_set_good(conn);
					sess->state = SMB2_SESSION_VALID;
				}
				if (conn->binding) {
					struct preauth_session *preauth_sess;

					preauth_sess =
						ksmbd_preauth_session_lookup(conn, sess->id);
					if (preauth_sess) {
						list_del(&preauth_sess->preauth_entry);
						kfree(preauth_sess);
					}
				}
				kfree(sess->Preauth_HashValue);
				sess->Preauth_HashValue = NULL;
			} else {
				pr_info_ratelimited("Unknown NTLMSSP message type : 0x%x\n",
						le32_to_cpu(negblob->MessageType));
				rc = -EINVAL;
			}
		} else {
			/* TODO: need one more negotiation */
			pr_err("Not support the preferred authentication\n");
			rc = -EINVAL;
		}
	} else {
		pr_err("Not support authentication\n");
		rc = -EINVAL;
	}

out_err:
	if (rc == -EINVAL)
		rsp->hdr.Status = STATUS_INVALID_PARAMETER;
	else if (rc == -ENOENT)
		rsp->hdr.Status = STATUS_USER_SESSION_DELETED;
	else if (rc == -EACCES)
		rsp->hdr.Status = STATUS_REQUEST_NOT_ACCEPTED;
	else if (rc == -EFAULT)
		rsp->hdr.Status = STATUS_NETWORK_SESSION_EXPIRED;
	else if (rc == -ENOMEM)
		rsp->hdr.Status = STATUS_INSUFFICIENT_RESOURCES;
	else if (rc)
		rsp->hdr.Status = STATUS_LOGON_FAILURE;

	if (conn->use_spnego && conn->mechToken) {
		kfree(conn->mechToken);
		conn->mechToken = NULL;
	}

	if (rc < 0) {
		/*
		 * SecurityBufferOffset should be set to zero
		 * in session setup error response.
		 */
		rsp->SecurityBufferOffset = 0;

		if (sess) {
			bool try_delay = false;

			/*
			 * To avoid dictionary attacks (repeated session setups rapidly sent) to
			 * connect to server, ksmbd make a delay of a 5 seconds on session setup
			 * failure to make it harder to send enough random connection requests
			 * to break into a server.
			 */
			if (sess->user && sess->user->flags & KSMBD_USER_FLAG_DELAY_SESSION)
				try_delay = true;

			sess->last_active = jiffies;
			sess->state = SMB2_SESSION_EXPIRED;
			if (try_delay)
				ssleep(5);
		}
	}

	ksmbd_conn_unlock(conn);
	return rc;
}
