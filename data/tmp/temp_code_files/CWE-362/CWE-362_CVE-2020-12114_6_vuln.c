static void drop_mountpoint(struct fs_pin *p)
{
	struct mount *m = container_of(p, struct mount, mnt_umount);
	dput(m->mnt_ex_mountpoint);
	pin_remove(p);
	mntput(&m->mnt);
}