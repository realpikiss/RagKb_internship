static void xfrm_update_ae_params(struct xfrm_state *x, struct nlattr **attrs,
				  int update_esn)
{
	struct nlattr *rp = attrs[XFRMA_REPLAY_VAL];
	struct nlattr *re = update_esn ? attrs[XFRMA_REPLAY_ESN_VAL] : NULL;
	struct nlattr *lt = attrs[XFRMA_LTIME_VAL];
	struct nlattr *et = attrs[XFRMA_ETIMER_THRESH];
	struct nlattr *rt = attrs[XFRMA_REPLAY_THRESH];
	struct nlattr *mt = attrs[XFRMA_MTIMER_THRESH];

	if (re && x->replay_esn && x->preplay_esn) {
		struct xfrm_replay_state_esn *replay_esn;
		replay_esn = nla_data(re);
		memcpy(x->replay_esn, replay_esn,
		       xfrm_replay_state_esn_len(replay_esn));
		memcpy(x->preplay_esn, replay_esn,
		       xfrm_replay_state_esn_len(replay_esn));
	}

	if (rp) {
		struct xfrm_replay_state *replay;
		replay = nla_data(rp);
		memcpy(&x->replay, replay, sizeof(*replay));
		memcpy(&x->preplay, replay, sizeof(*replay));
	}

	if (lt) {
		struct xfrm_lifetime_cur *ltime;
		ltime = nla_data(lt);
		x->curlft.bytes = ltime->bytes;
		x->curlft.packets = ltime->packets;
		x->curlft.add_time = ltime->add_time;
		x->curlft.use_time = ltime->use_time;
	}

	if (et)
		x->replay_maxage = nla_get_u32(et);

	if (rt)
		x->replay_maxdiff = nla_get_u32(rt);

	if (mt)
		x->mapping_maxage = nla_get_u32(mt);
}
