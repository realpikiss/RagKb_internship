static struct l2cap_chan *l2cap_get_chan_by_scid(struct l2cap_conn *conn,
						 u16 cid)
{
	struct l2cap_chan *c;

	mutex_lock(&conn->chan_lock);
	c = __l2cap_get_chan_by_scid(conn, cid);
	if (c) {
		/* Only lock if chan reference is not 0 */
		c = l2cap_chan_hold_unless_zero(c);
		if (c)
			l2cap_chan_lock(c);
	}
	mutex_unlock(&conn->chan_lock);

	return c;
}
