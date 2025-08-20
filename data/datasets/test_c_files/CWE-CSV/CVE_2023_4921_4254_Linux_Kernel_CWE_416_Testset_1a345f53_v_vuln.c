static struct sk_buff *qfq_dequeue(struct Qdisc *sch)
{
	struct qfq_sched *q = qdisc_priv(sch);
	struct qfq_aggregate *in_serv_agg = q->in_serv_agg;
	struct qfq_class *cl;
	struct sk_buff *skb = NULL;
	/* next-packet len, 0 means no more active classes in in-service agg */
	unsigned int len = 0;

	if (in_serv_agg == NULL)
		return NULL;

	if (!list_empty(&in_serv_agg->active))
		skb = qfq_peek_skb(in_serv_agg, &cl, &len);

	/*
	 * If there are no active classes in the in-service aggregate,
	 * or if the aggregate has not enough budget to serve its next
	 * class, then choose the next aggregate to serve.
	 */
	if (len == 0 || in_serv_agg->budget < len) {
		charge_actual_service(in_serv_agg);

		/* recharge the budget of the aggregate */
		in_serv_agg->initial_budget = in_serv_agg->budget =
			in_serv_agg->budgetmax;

		if (!list_empty(&in_serv_agg->active)) {
			/*
			 * Still active: reschedule for
			 * service. Possible optimization: if no other
			 * aggregate is active, then there is no point
			 * in rescheduling this aggregate, and we can
			 * just keep it as the in-service one. This
			 * should be however a corner case, and to
			 * handle it, we would need to maintain an
			 * extra num_active_aggs field.
			*/
			qfq_update_agg_ts(q, in_serv_agg, requeue);
			qfq_schedule_agg(q, in_serv_agg);
		} else if (sch->q.qlen == 0) { /* no aggregate to serve */
			q->in_serv_agg = NULL;
			return NULL;
		}

		/*
		 * If we get here, there are other aggregates queued:
		 * choose the new aggregate to serve.
		 */
		in_serv_agg = q->in_serv_agg = qfq_choose_next_agg(q);
		skb = qfq_peek_skb(in_serv_agg, &cl, &len);
	}
	if (!skb)
		return NULL;

	qdisc_qstats_backlog_dec(sch, skb);
	sch->q.qlen--;
	qdisc_bstats_update(sch, skb);

	agg_dequeue(in_serv_agg, cl, len);
	/* If lmax is lowered, through qfq_change_class, for a class
	 * owning pending packets with larger size than the new value
	 * of lmax, then the following condition may hold.
	 */
	if (unlikely(in_serv_agg->budget < len))
		in_serv_agg->budget = 0;
	else
		in_serv_agg->budget -= len;

	q->V += (u64)len * q->iwsum;
	pr_debug("qfq dequeue: len %u F %lld now %lld\n",
		 len, (unsigned long long) in_serv_agg->F,
		 (unsigned long long) q->V);

	return skb;
}
