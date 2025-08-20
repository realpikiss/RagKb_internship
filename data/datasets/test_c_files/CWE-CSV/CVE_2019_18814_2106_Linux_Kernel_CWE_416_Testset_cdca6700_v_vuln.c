int aa_audit_rule_init(u32 field, u32 op, char *rulestr, void **vrule)
{
	struct aa_audit_rule *rule;

	switch (field) {
	case AUDIT_SUBJ_ROLE:
		if (op != Audit_equal && op != Audit_not_equal)
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}

	rule = kzalloc(sizeof(struct aa_audit_rule), GFP_KERNEL);

	if (!rule)
		return -ENOMEM;

	/* Currently rules are treated as coming from the root ns */
	rule->label = aa_label_parse(&root_ns->unconfined->label, rulestr,
				     GFP_KERNEL, true, false);
	if (IS_ERR(rule->label)) {
		aa_audit_rule_free(rule);
		return PTR_ERR(rule->label);
	}

	*vrule = rule;
	return 0;
}
