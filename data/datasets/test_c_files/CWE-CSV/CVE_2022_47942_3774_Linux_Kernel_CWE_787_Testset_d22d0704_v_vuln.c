int smb_inherit_dacl(struct ksmbd_conn *conn,
		     struct path *path,
		     unsigned int uid, unsigned int gid)
{
	const struct smb_sid *psid, *creator = NULL;
	struct smb_ace *parent_aces, *aces;
	struct smb_acl *parent_pdacl;
	struct smb_ntsd *parent_pntsd = NULL;
	struct smb_sid owner_sid, group_sid;
	struct dentry *parent = path->dentry->d_parent;
	struct user_namespace *user_ns = mnt_user_ns(path->mnt);
	int inherited_flags = 0, flags = 0, i, ace_cnt = 0, nt_size = 0;
	int rc = 0, num_aces, dacloffset, pntsd_type, acl_len;
	char *aces_base;
	bool is_dir = S_ISDIR(d_inode(path->dentry)->i_mode);

	acl_len = ksmbd_vfs_get_sd_xattr(conn, user_ns,
					 parent, &parent_pntsd);
	if (acl_len <= 0)
		return -ENOENT;
	dacloffset = le32_to_cpu(parent_pntsd->dacloffset);
	if (!dacloffset) {
		rc = -EINVAL;
		goto free_parent_pntsd;
	}

	parent_pdacl = (struct smb_acl *)((char *)parent_pntsd + dacloffset);
	num_aces = le32_to_cpu(parent_pdacl->num_aces);
	pntsd_type = le16_to_cpu(parent_pntsd->type);

	aces_base = kmalloc(sizeof(struct smb_ace) * num_aces * 2, GFP_KERNEL);
	if (!aces_base) {
		rc = -ENOMEM;
		goto free_parent_pntsd;
	}

	aces = (struct smb_ace *)aces_base;
	parent_aces = (struct smb_ace *)((char *)parent_pdacl +
			sizeof(struct smb_acl));

	if (pntsd_type & DACL_AUTO_INHERITED)
		inherited_flags = INHERITED_ACE;

	for (i = 0; i < num_aces; i++) {
		flags = parent_aces->flags;
		if (!smb_inherit_flags(flags, is_dir))
			goto pass;
		if (is_dir) {
			flags &= ~(INHERIT_ONLY_ACE | INHERITED_ACE);
			if (!(flags & CONTAINER_INHERIT_ACE))
				flags |= INHERIT_ONLY_ACE;
			if (flags & NO_PROPAGATE_INHERIT_ACE)
				flags = 0;
		} else {
			flags = 0;
		}

		if (!compare_sids(&creator_owner, &parent_aces->sid)) {
			creator = &creator_owner;
			id_to_sid(uid, SIDOWNER, &owner_sid);
			psid = &owner_sid;
		} else if (!compare_sids(&creator_group, &parent_aces->sid)) {
			creator = &creator_group;
			id_to_sid(gid, SIDUNIX_GROUP, &group_sid);
			psid = &group_sid;
		} else {
			creator = NULL;
			psid = &parent_aces->sid;
		}

		if (is_dir && creator && flags & CONTAINER_INHERIT_ACE) {
			smb_set_ace(aces, psid, parent_aces->type, inherited_flags,
				    parent_aces->access_req);
			nt_size += le16_to_cpu(aces->size);
			ace_cnt++;
			aces = (struct smb_ace *)((char *)aces + le16_to_cpu(aces->size));
			flags |= INHERIT_ONLY_ACE;
			psid = creator;
		} else if (is_dir && !(parent_aces->flags & NO_PROPAGATE_INHERIT_ACE)) {
			psid = &parent_aces->sid;
		}

		smb_set_ace(aces, psid, parent_aces->type, flags | inherited_flags,
			    parent_aces->access_req);
		nt_size += le16_to_cpu(aces->size);
		aces = (struct smb_ace *)((char *)aces + le16_to_cpu(aces->size));
		ace_cnt++;
pass:
		parent_aces =
			(struct smb_ace *)((char *)parent_aces + le16_to_cpu(parent_aces->size));
	}

	if (nt_size > 0) {
		struct smb_ntsd *pntsd;
		struct smb_acl *pdacl;
		struct smb_sid *powner_sid = NULL, *pgroup_sid = NULL;
		int powner_sid_size = 0, pgroup_sid_size = 0, pntsd_size;

		if (parent_pntsd->osidoffset) {
			powner_sid = (struct smb_sid *)((char *)parent_pntsd +
					le32_to_cpu(parent_pntsd->osidoffset));
			powner_sid_size = 1 + 1 + 6 + (powner_sid->num_subauth * 4);
		}
		if (parent_pntsd->gsidoffset) {
			pgroup_sid = (struct smb_sid *)((char *)parent_pntsd +
					le32_to_cpu(parent_pntsd->gsidoffset));
			pgroup_sid_size = 1 + 1 + 6 + (pgroup_sid->num_subauth * 4);
		}

		pntsd = kzalloc(sizeof(struct smb_ntsd) + powner_sid_size +
				pgroup_sid_size + sizeof(struct smb_acl) +
				nt_size, GFP_KERNEL);
		if (!pntsd) {
			rc = -ENOMEM;
			goto free_aces_base;
		}

		pntsd->revision = cpu_to_le16(1);
		pntsd->type = cpu_to_le16(SELF_RELATIVE | DACL_PRESENT);
		if (le16_to_cpu(parent_pntsd->type) & DACL_AUTO_INHERITED)
			pntsd->type |= cpu_to_le16(DACL_AUTO_INHERITED);
		pntsd_size = sizeof(struct smb_ntsd);
		pntsd->osidoffset = parent_pntsd->osidoffset;
		pntsd->gsidoffset = parent_pntsd->gsidoffset;
		pntsd->dacloffset = parent_pntsd->dacloffset;

		if (pntsd->osidoffset) {
			struct smb_sid *owner_sid = (struct smb_sid *)((char *)pntsd +
					le32_to_cpu(pntsd->osidoffset));
			memcpy(owner_sid, powner_sid, powner_sid_size);
			pntsd_size += powner_sid_size;
		}

		if (pntsd->gsidoffset) {
			struct smb_sid *group_sid = (struct smb_sid *)((char *)pntsd +
					le32_to_cpu(pntsd->gsidoffset));
			memcpy(group_sid, pgroup_sid, pgroup_sid_size);
			pntsd_size += pgroup_sid_size;
		}

		if (pntsd->dacloffset) {
			struct smb_ace *pace;

			pdacl = (struct smb_acl *)((char *)pntsd + le32_to_cpu(pntsd->dacloffset));
			pdacl->revision = cpu_to_le16(2);
			pdacl->size = cpu_to_le16(sizeof(struct smb_acl) + nt_size);
			pdacl->num_aces = cpu_to_le32(ace_cnt);
			pace = (struct smb_ace *)((char *)pdacl + sizeof(struct smb_acl));
			memcpy(pace, aces_base, nt_size);
			pntsd_size += sizeof(struct smb_acl) + nt_size;
		}

		ksmbd_vfs_set_sd_xattr(conn, user_ns,
				       path->dentry, pntsd, pntsd_size);
		kfree(pntsd);
	}

free_aces_base:
	kfree(aces_base);
free_parent_pntsd:
	kfree(parent_pntsd);
	return rc;
}
