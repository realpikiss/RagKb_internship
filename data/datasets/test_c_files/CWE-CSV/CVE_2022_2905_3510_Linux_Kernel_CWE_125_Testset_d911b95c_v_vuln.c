static int
record_func_key(struct bpf_verifier_env *env, struct bpf_call_arg_meta *meta,
		int func_id, int insn_idx)
{
	struct bpf_insn_aux_data *aux = &env->insn_aux_data[insn_idx];
	struct bpf_reg_state *regs = cur_regs(env), *reg;
	struct bpf_map *map = meta->map_ptr;
	struct tnum range;
	u64 val;
	int err;

	if (func_id != BPF_FUNC_tail_call)
		return 0;
	if (!map || map->map_type != BPF_MAP_TYPE_PROG_ARRAY) {
		verbose(env, "kernel subsystem misconfigured verifier\n");
		return -EINVAL;
	}

	range = tnum_range(0, map->max_entries - 1);
	reg = &regs[BPF_REG_3];

	if (!register_is_const(reg) || !tnum_in(range, reg->var_off)) {
		bpf_map_key_store(aux, BPF_MAP_KEY_POISON);
		return 0;
	}

	err = mark_chain_precision(env, BPF_REG_3);
	if (err)
		return err;

	val = reg->var_off.value;
	if (bpf_map_key_unseen(aux))
		bpf_map_key_store(aux, val);
	else if (!bpf_map_key_poisoned(aux) &&
		  bpf_map_key_immediate(aux) != val)
		bpf_map_key_store(aux, BPF_MAP_KEY_POISON);
	return 0;
}
