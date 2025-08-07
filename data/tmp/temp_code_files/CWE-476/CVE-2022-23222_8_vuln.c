static int check_return_code(struct bpf_verifier_env *env)
{
	struct tnum enforce_attach_type_range = tnum_unknown;
	const struct bpf_prog *prog = env->prog;
	struct bpf_reg_state *reg;
	struct tnum range = tnum_range(0, 1);
	enum bpf_prog_type prog_type = resolve_prog_type(env->prog);
	int err;
	struct bpf_func_state *frame = env->cur_state->frame[0];
	const bool is_subprog = frame->subprogno;

	/* LSM and struct_ops func-ptr's return type could be "void" */
	if (!is_subprog &&
	    (prog_type == BPF_PROG_TYPE_STRUCT_OPS ||
	     prog_type == BPF_PROG_TYPE_LSM) &&
	    !prog->aux->attach_func_proto->type)
		return 0;

	/* eBPF calling convention is such that R0 is used
	 * to return the value from eBPF program.
	 * Make sure that it's readable at this time
	 * of bpf_exit, which means that program wrote
	 * something into it earlier
	 */
	err = check_reg_arg(env, BPF_REG_0, SRC_OP);
	if (err)
		return err;

	if (is_pointer_value(env, BPF_REG_0)) {
		verbose(env, "R0 leaks addr as return value\n");
		return -EACCES;
	}

	reg = cur_regs(env) + BPF_REG_0;

	if (frame->in_async_callback_fn) {
		/* enforce return zero from async callbacks like timer */
		if (reg->type != SCALAR_VALUE) {
			verbose(env, "In async callback the register R0 is not a known value (%s)\n",
				reg_type_str[reg->type]);
			return -EINVAL;
		}

		if (!tnum_in(tnum_const(0), reg->var_off)) {
			verbose_invalid_scalar(env, reg, &range, "async callback", "R0");
			return -EINVAL;
		}
		return 0;
	}

	if (is_subprog) {
		if (reg->type != SCALAR_VALUE) {
			verbose(env, "At subprogram exit the register R0 is not a scalar value (%s)\n",
				reg_type_str[reg->type]);
			return -EINVAL;
		}
		return 0;
	}

	switch (prog_type) {
	case BPF_PROG_TYPE_CGROUP_SOCK_ADDR:
		if (env->prog->expected_attach_type == BPF_CGROUP_UDP4_RECVMSG ||
		    env->prog->expected_attach_type == BPF_CGROUP_UDP6_RECVMSG ||
		    env->prog->expected_attach_type == BPF_CGROUP_INET4_GETPEERNAME ||
		    env->prog->expected_attach_type == BPF_CGROUP_INET6_GETPEERNAME ||
		    env->prog->expected_attach_type == BPF_CGROUP_INET4_GETSOCKNAME ||
		    env->prog->expected_attach_type == BPF_CGROUP_INET6_GETSOCKNAME)
			range = tnum_range(1, 1);
		if (env->prog->expected_attach_type == BPF_CGROUP_INET4_BIND ||
		    env->prog->expected_attach_type == BPF_CGROUP_INET6_BIND)
			range = tnum_range(0, 3);
		break;
	case BPF_PROG_TYPE_CGROUP_SKB:
		if (env->prog->expected_attach_type == BPF_CGROUP_INET_EGRESS) {
			range = tnum_range(0, 3);
			enforce_attach_type_range = tnum_range(2, 3);
		}
		break;
	case BPF_PROG_TYPE_CGROUP_SOCK:
	case BPF_PROG_TYPE_SOCK_OPS:
	case BPF_PROG_TYPE_CGROUP_DEVICE:
	case BPF_PROG_TYPE_CGROUP_SYSCTL:
	case BPF_PROG_TYPE_CGROUP_SOCKOPT:
		break;
	case BPF_PROG_TYPE_RAW_TRACEPOINT:
		if (!env->prog->aux->attach_btf_id)
			return 0;
		range = tnum_const(0);
		break;
	case BPF_PROG_TYPE_TRACING:
		switch (env->prog->expected_attach_type) {
		case BPF_TRACE_FENTRY:
		case BPF_TRACE_FEXIT:
			range = tnum_const(0);
			break;
		case BPF_TRACE_RAW_TP:
		case BPF_MODIFY_RETURN:
			return 0;
		case BPF_TRACE_ITER:
			break;
		default:
			return -ENOTSUPP;
		}
		break;
	case BPF_PROG_TYPE_SK_LOOKUP:
		range = tnum_range(SK_DROP, SK_PASS);
		break;
	case BPF_PROG_TYPE_EXT:
		/* freplace program can return anything as its return value
		 * depends on the to-be-replaced kernel func or bpf program.
		 */
	default:
		return 0;
	}

	if (reg->type != SCALAR_VALUE) {
		verbose(env, "At program exit the register R0 is not a known value (%s)\n",
			reg_type_str[reg->type]);
		return -EINVAL;
	}

	if (!tnum_in(range, reg->var_off)) {
		verbose_invalid_scalar(env, reg, &range, "program exit", "R0");
		return -EINVAL;
	}

	if (!tnum_is_unknown(enforce_attach_type_range) &&
	    tnum_in(enforce_attach_type_range, reg->var_off))
		env->prog->enforce_expected_attach_type = 1;
	return 0;
}