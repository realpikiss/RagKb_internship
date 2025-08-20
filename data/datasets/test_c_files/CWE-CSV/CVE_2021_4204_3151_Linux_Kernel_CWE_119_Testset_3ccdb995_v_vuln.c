static int btf_check_func_arg_match(struct bpf_verifier_env *env,
				    const struct btf *btf, u32 func_id,
				    struct bpf_reg_state *regs,
				    bool ptr_to_mem_ok)
{
	struct bpf_verifier_log *log = &env->log;
	bool is_kfunc = btf_is_kernel(btf);
	const char *func_name, *ref_tname;
	const struct btf_type *t, *ref_t;
	const struct btf_param *args;
	u32 i, nargs, ref_id;

	t = btf_type_by_id(btf, func_id);
	if (!t || !btf_type_is_func(t)) {
		/* These checks were already done by the verifier while loading
		 * struct bpf_func_info or in add_kfunc_call().
		 */
		bpf_log(log, "BTF of func_id %u doesn't point to KIND_FUNC\n",
			func_id);
		return -EFAULT;
	}
	func_name = btf_name_by_offset(btf, t->name_off);

	t = btf_type_by_id(btf, t->type);
	if (!t || !btf_type_is_func_proto(t)) {
		bpf_log(log, "Invalid BTF of func %s\n", func_name);
		return -EFAULT;
	}
	args = (const struct btf_param *)(t + 1);
	nargs = btf_type_vlen(t);
	if (nargs > MAX_BPF_FUNC_REG_ARGS) {
		bpf_log(log, "Function %s has %d > %d args\n", func_name, nargs,
			MAX_BPF_FUNC_REG_ARGS);
		return -EINVAL;
	}

	/* check that BTF function arguments match actual types that the
	 * verifier sees.
	 */
	for (i = 0; i < nargs; i++) {
		u32 regno = i + 1;
		struct bpf_reg_state *reg = &regs[regno];

		t = btf_type_skip_modifiers(btf, args[i].type, NULL);
		if (btf_type_is_scalar(t)) {
			if (reg->type == SCALAR_VALUE)
				continue;
			bpf_log(log, "R%d is not a scalar\n", regno);
			return -EINVAL;
		}

		if (!btf_type_is_ptr(t)) {
			bpf_log(log, "Unrecognized arg#%d type %s\n",
				i, btf_type_str(t));
			return -EINVAL;
		}

		ref_t = btf_type_skip_modifiers(btf, t->type, &ref_id);
		ref_tname = btf_name_by_offset(btf, ref_t->name_off);
		if (btf_get_prog_ctx_type(log, btf, t,
					  env->prog->type, i)) {
			/* If function expects ctx type in BTF check that caller
			 * is passing PTR_TO_CTX.
			 */
			if (reg->type != PTR_TO_CTX) {
				bpf_log(log,
					"arg#%d expected pointer to ctx, but got %s\n",
					i, btf_type_str(t));
				return -EINVAL;
			}
			if (check_ctx_reg(env, reg, regno))
				return -EINVAL;
		} else if (is_kfunc && (reg->type == PTR_TO_BTF_ID || reg2btf_ids[reg->type])) {
			const struct btf_type *reg_ref_t;
			const struct btf *reg_btf;
			const char *reg_ref_tname;
			u32 reg_ref_id;

			if (!btf_type_is_struct(ref_t)) {
				bpf_log(log, "kernel function %s args#%d pointer type %s %s is not supported\n",
					func_name, i, btf_type_str(ref_t),
					ref_tname);
				return -EINVAL;
			}

			if (reg->type == PTR_TO_BTF_ID) {
				reg_btf = reg->btf;
				reg_ref_id = reg->btf_id;
			} else {
				reg_btf = btf_vmlinux;
				reg_ref_id = *reg2btf_ids[reg->type];
			}

			reg_ref_t = btf_type_skip_modifiers(reg_btf, reg_ref_id,
							    &reg_ref_id);
			reg_ref_tname = btf_name_by_offset(reg_btf,
							   reg_ref_t->name_off);
			if (!btf_struct_ids_match(log, reg_btf, reg_ref_id,
						  reg->off, btf, ref_id)) {
				bpf_log(log, "kernel function %s args#%d expected pointer to %s %s but R%d has a pointer to %s %s\n",
					func_name, i,
					btf_type_str(ref_t), ref_tname,
					regno, btf_type_str(reg_ref_t),
					reg_ref_tname);
				return -EINVAL;
			}
		} else if (ptr_to_mem_ok) {
			const struct btf_type *resolve_ret;
			u32 type_size;

			if (is_kfunc) {
				/* Permit pointer to mem, but only when argument
				 * type is pointer to scalar, or struct composed
				 * (recursively) of scalars.
				 */
				if (!btf_type_is_scalar(ref_t) &&
				    !__btf_type_is_scalar_struct(log, btf, ref_t, 0)) {
					bpf_log(log,
						"arg#%d pointer type %s %s must point to scalar or struct with scalar\n",
						i, btf_type_str(ref_t), ref_tname);
					return -EINVAL;
				}
			}

			resolve_ret = btf_resolve_size(btf, ref_t, &type_size);
			if (IS_ERR(resolve_ret)) {
				bpf_log(log,
					"arg#%d reference type('%s %s') size cannot be determined: %ld\n",
					i, btf_type_str(ref_t), ref_tname,
					PTR_ERR(resolve_ret));
				return -EINVAL;
			}

			if (check_mem_reg(env, reg, regno, type_size))
				return -EINVAL;
		} else {
			bpf_log(log, "reg type unsupported for arg#%d %sfunction %s#%d\n", i,
				is_kfunc ? "kernel " : "", func_name, func_id);
			return -EINVAL;
		}
	}

	return 0;
}
