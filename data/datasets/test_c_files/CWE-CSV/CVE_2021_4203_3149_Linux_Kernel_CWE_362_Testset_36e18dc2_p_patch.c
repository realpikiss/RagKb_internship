static void copy_peercred(struct sock *sk, struct sock *peersk)
{
	const struct cred *old_cred;
	struct pid *old_pid;

	if (sk < peersk) {
		spin_lock(&sk->sk_peer_lock);
		spin_lock_nested(&peersk->sk_peer_lock, SINGLE_DEPTH_NESTING);
	} else {
		spin_lock(&peersk->sk_peer_lock);
		spin_lock_nested(&sk->sk_peer_lock, SINGLE_DEPTH_NESTING);
	}
	old_pid = sk->sk_peer_pid;
	old_cred = sk->sk_peer_cred;
	sk->sk_peer_pid  = get_pid(peersk->sk_peer_pid);
	sk->sk_peer_cred = get_cred(peersk->sk_peer_cred);

	spin_unlock(&sk->sk_peer_lock);
	spin_unlock(&peersk->sk_peer_lock);

	put_pid(old_pid);
	put_cred(old_cred);
}
