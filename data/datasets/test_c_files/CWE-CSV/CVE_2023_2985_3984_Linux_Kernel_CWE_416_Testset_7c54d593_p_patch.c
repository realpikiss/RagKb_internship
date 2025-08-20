static void hfsplus_put_super(struct super_block *sb)
{
	struct hfsplus_sb_info *sbi = HFSPLUS_SB(sb);

	hfs_dbg(SUPER, "hfsplus_put_super\n");

	cancel_delayed_work_sync(&sbi->sync_work);

	if (!sb_rdonly(sb) && sbi->s_vhdr) {
		struct hfsplus_vh *vhdr = sbi->s_vhdr;

		vhdr->modify_date = hfsp_now2mt();
		vhdr->attributes |= cpu_to_be32(HFSPLUS_VOL_UNMNT);
		vhdr->attributes &= cpu_to_be32(~HFSPLUS_VOL_INCNSTNT);

		hfsplus_sync_fs(sb, 1);
	}

	iput(sbi->alloc_file);
	iput(sbi->hidden_dir);
	hfs_btree_close(sbi->attr_tree);
	hfs_btree_close(sbi->cat_tree);
	hfs_btree_close(sbi->ext_tree);
	kfree(sbi->s_vhdr_buf);
	kfree(sbi->s_backup_vhdr_buf);
	unload_nls(sbi->nls);
	kfree(sb->s_fs_info);
	sb->s_fs_info = NULL;
}
