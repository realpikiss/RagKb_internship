int check_mem_reg(struct bpf_verifier_env *env, struct bpf_reg_state *reg,
		   u32 regno, u32 mem_size)
{
	if (register_is_null(reg))
		return 0;

	if (type_may_be_null(reg->type)) {
		/* Assuming that the register contains a value check if the memory
		 * access is safe. Temporarily save and restore the register's state as
		 * the conversion shouldn't be visible to a caller.
		 */
		const struct bpf_reg_state saved_reg = *reg;
		int rv;

		mark_ptr_not_null_reg(reg);
		rv = check_helper_mem_access(env, regno, mem_size, true, NULL);
		*reg = saved_reg;
		return rv;
	}

	return check_helper_mem_access(env, regno, mem_size, true, NULL);
}