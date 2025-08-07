static void print_verifier_state(struct bpf_verifier_env *env,
				 const struct bpf_func_state *state,
				 bool print_all)
{
	const struct bpf_reg_state *reg;
	enum bpf_reg_type t;
	int i;

	if (state->frameno)
		verbose(env, " frame%d:", state->frameno);
	for (i = 0; i < MAX_BPF_REG; i++) {
		reg = &state->regs[i];
		t = reg->type;
		if (t == NOT_INIT)
			continue;
		if (!print_all && !reg_scratched(env, i))
			continue;
		verbose(env, " R%d", i);
		print_liveness(env, reg->live);
		verbose(env, "=%s", reg_type_str(env, t));
		if (t == SCALAR_VALUE && reg->precise)
			verbose(env, "P");
		if ((t == SCALAR_VALUE || t == PTR_TO_STACK) &&
		    tnum_is_const(reg->var_off)) {
			/* reg->off should be 0 for SCALAR_VALUE */
			verbose(env, "%lld", reg->var_off.value + reg->off);
		} else {
			if (base_type(t) == PTR_TO_BTF_ID ||
			    base_type(t) == PTR_TO_PERCPU_BTF_ID)
				verbose(env, "%s", kernel_type_name(reg->btf, reg->btf_id));
			verbose(env, "(id=%d", reg->id);
			if (reg_type_may_be_refcounted_or_null(t))
				verbose(env, ",ref_obj_id=%d", reg->ref_obj_id);
			if (t != SCALAR_VALUE)
				verbose(env, ",off=%d", reg->off);
			if (type_is_pkt_pointer(t))
				verbose(env, ",r=%d", reg->range);
			else if (base_type(t) == CONST_PTR_TO_MAP ||
				 base_type(t) == PTR_TO_MAP_KEY ||
				 base_type(t) == PTR_TO_MAP_VALUE)
				verbose(env, ",ks=%d,vs=%d",
					reg->map_ptr->key_size,
					reg->map_ptr->value_size);
			if (tnum_is_const(reg->var_off)) {
				/* Typically an immediate SCALAR_VALUE, but
				 * could be a pointer whose offset is too big
				 * for reg->off
				 */
				verbose(env, ",imm=%llx", reg->var_off.value);
			} else {
				if (reg->smin_value != reg->umin_value &&
				    reg->smin_value != S64_MIN)
					verbose(env, ",smin_value=%lld",
						(long long)reg->smin_value);
				if (reg->smax_value != reg->umax_value &&
				    reg->smax_value != S64_MAX)
					verbose(env, ",smax_value=%lld",
						(long long)reg->smax_value);
				if (reg->umin_value != 0)
					verbose(env, ",umin_value=%llu",
						(unsigned long long)reg->umin_value);
				if (reg->umax_value != U64_MAX)
					verbose(env, ",umax_value=%llu",
						(unsigned long long)reg->umax_value);
				if (!tnum_is_unknown(reg->var_off)) {
					char tn_buf[48];

					tnum_strn(tn_buf, sizeof(tn_buf), reg->var_off);
					verbose(env, ",var_off=%s", tn_buf);
				}
				if (reg->s32_min_value != reg->smin_value &&
				    reg->s32_min_value != S32_MIN)
					verbose(env, ",s32_min_value=%d",
						(int)(reg->s32_min_value));
				if (reg->s32_max_value != reg->smax_value &&
				    reg->s32_max_value != S32_MAX)
					verbose(env, ",s32_max_value=%d",
						(int)(reg->s32_max_value));
				if (reg->u32_min_value != reg->umin_value &&
				    reg->u32_min_value != U32_MIN)
					verbose(env, ",u32_min_value=%d",
						(int)(reg->u32_min_value));
				if (reg->u32_max_value != reg->umax_value &&
				    reg->u32_max_value != U32_MAX)
					verbose(env, ",u32_max_value=%d",
						(int)(reg->u32_max_value));
			}
			verbose(env, ")");
		}
	}
	for (i = 0; i < state->allocated_stack / BPF_REG_SIZE; i++) {
		char types_buf[BPF_REG_SIZE + 1];
		bool valid = false;
		int j;

		for (j = 0; j < BPF_REG_SIZE; j++) {
			if (state->stack[i].slot_type[j] != STACK_INVALID)
				valid = true;
			types_buf[j] = slot_type_char[
					state->stack[i].slot_type[j]];
		}
		types_buf[BPF_REG_SIZE] = 0;
		if (!valid)
			continue;
		if (!print_all && !stack_slot_scratched(env, i))
			continue;
		verbose(env, " fp%d", (-i - 1) * BPF_REG_SIZE);
		print_liveness(env, state->stack[i].spilled_ptr.live);
		if (is_spilled_reg(&state->stack[i])) {
			reg = &state->stack[i].spilled_ptr;
			t = reg->type;
			verbose(env, "=%s", reg_type_str(env, t));
			if (t == SCALAR_VALUE && reg->precise)
				verbose(env, "P");
			if (t == SCALAR_VALUE && tnum_is_const(reg->var_off))
				verbose(env, "%lld", reg->var_off.value + reg->off);
		} else {
			verbose(env, "=%s", types_buf);
		}
	}
	if (state->acquired_refs && state->refs[0].id) {
		verbose(env, " refs=%d", state->refs[0].id);
		for (i = 1; i < state->acquired_refs; i++)
			if (state->refs[i].id)
				verbose(env, ",%d", state->refs[i].id);
	}
	if (state->in_callback_fn)
		verbose(env, " cb");
	if (state->in_async_callback_fn)
		verbose(env, " async_cb");
	verbose(env, "\n");
	mark_verifier_state_clean(env);
}