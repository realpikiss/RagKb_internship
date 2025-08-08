static void umount_mnt(struct mount *mnt)
{
	put_mountpoint(unhash_mnt(mnt));
}