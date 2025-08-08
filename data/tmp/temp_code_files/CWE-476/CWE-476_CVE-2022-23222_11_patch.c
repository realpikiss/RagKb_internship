static bool reg_type_may_be_refcounted_or_null(enum bpf_reg_type type)
{
	return base_type(type) == PTR_TO_SOCKET ||
		base_type(type) == PTR_TO_TCP_SOCK ||
		base_type(type) == PTR_TO_MEM;
}