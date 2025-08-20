static int smb2_get_data_area_len(unsigned int *off, unsigned int *len,
				  struct smb2_hdr *hdr)
{
	int ret = 0;

	*off = 0;
	*len = 0;

	/* error reqeusts do not have data area */
	if (hdr->Status && hdr->Status != STATUS_MORE_PROCESSING_REQUIRED &&
	    (((struct smb2_err_rsp *)hdr)->StructureSize) == SMB2_ERROR_STRUCTURE_SIZE2_LE)
		return ret;

	/*
	 * Following commands have data areas so we have to get the location
	 * of the data buffer offset and data buffer length for the particular
	 * command.
	 */
	switch (hdr->Command) {
	case SMB2_SESSION_SETUP:
		*off = le16_to_cpu(((struct smb2_sess_setup_req *)hdr)->SecurityBufferOffset);
		*len = le16_to_cpu(((struct smb2_sess_setup_req *)hdr)->SecurityBufferLength);
		break;
	case SMB2_TREE_CONNECT:
		*off = le16_to_cpu(((struct smb2_tree_connect_req *)hdr)->PathOffset);
		*len = le16_to_cpu(((struct smb2_tree_connect_req *)hdr)->PathLength);
		break;
	case SMB2_CREATE:
	{
		if (((struct smb2_create_req *)hdr)->CreateContextsLength) {
			*off = le32_to_cpu(((struct smb2_create_req *)
				hdr)->CreateContextsOffset);
			*len = le32_to_cpu(((struct smb2_create_req *)
				hdr)->CreateContextsLength);
			break;
		}

		*off = le16_to_cpu(((struct smb2_create_req *)hdr)->NameOffset);
		*len = le16_to_cpu(((struct smb2_create_req *)hdr)->NameLength);
		break;
	}
	case SMB2_QUERY_INFO:
		*off = le16_to_cpu(((struct smb2_query_info_req *)hdr)->InputBufferOffset);
		*len = le32_to_cpu(((struct smb2_query_info_req *)hdr)->InputBufferLength);
		break;
	case SMB2_SET_INFO:
		*off = le16_to_cpu(((struct smb2_set_info_req *)hdr)->BufferOffset);
		*len = le32_to_cpu(((struct smb2_set_info_req *)hdr)->BufferLength);
		break;
	case SMB2_READ:
		*off = le16_to_cpu(((struct smb2_read_req *)hdr)->ReadChannelInfoOffset);
		*len = le16_to_cpu(((struct smb2_read_req *)hdr)->ReadChannelInfoLength);
		break;
	case SMB2_WRITE:
		if (((struct smb2_write_req *)hdr)->DataOffset ||
		    ((struct smb2_write_req *)hdr)->Length) {
			*off = max_t(unsigned int,
				     le16_to_cpu(((struct smb2_write_req *)hdr)->DataOffset),
				     offsetof(struct smb2_write_req, Buffer));
			*len = le32_to_cpu(((struct smb2_write_req *)hdr)->Length);
			break;
		}

		*off = le16_to_cpu(((struct smb2_write_req *)hdr)->WriteChannelInfoOffset);
		*len = le16_to_cpu(((struct smb2_write_req *)hdr)->WriteChannelInfoLength);
		break;
	case SMB2_QUERY_DIRECTORY:
		*off = le16_to_cpu(((struct smb2_query_directory_req *)hdr)->FileNameOffset);
		*len = le16_to_cpu(((struct smb2_query_directory_req *)hdr)->FileNameLength);
		break;
	case SMB2_LOCK:
	{
		int lock_count;

		/*
		 * smb2_lock request size is 48 included single
		 * smb2_lock_element structure size.
		 */
		lock_count = le16_to_cpu(((struct smb2_lock_req *)hdr)->LockCount) - 1;
		if (lock_count > 0) {
			*off = __SMB2_HEADER_STRUCTURE_SIZE + 48;
			*len = sizeof(struct smb2_lock_element) * lock_count;
		}
		break;
	}
	case SMB2_IOCTL:
		*off = le32_to_cpu(((struct smb2_ioctl_req *)hdr)->InputOffset);
		*len = le32_to_cpu(((struct smb2_ioctl_req *)hdr)->InputCount);
		break;
	default:
		ksmbd_debug(SMB, "no length check for command\n");
		break;
	}

	if (*off > 4096) {
		ksmbd_debug(SMB, "offset %d too large\n", *off);
		ret = -EINVAL;
	} else if ((u64)*off + *len > MAX_STREAM_PROT_LEN) {
		ksmbd_debug(SMB, "Request is larger than maximum stream protocol length(%u): %llu\n",
			    MAX_STREAM_PROT_LEN, (u64)*off + *len);
		ret = -EINVAL;
	}

	return ret;
}
