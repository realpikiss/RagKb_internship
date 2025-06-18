static void mark_ptr_not_null_reg(struct bpf_reg_state *reg)
{
	switch (reg->type) {
	case PTR_TO_MAP_VALUE_OR_NULL: {
		const struct bpf_map *map = reg->map_ptr;

		if (map->inner_map_meta) {
			reg->type = CONST_PTR_TO_MAP;
			reg->map_ptr = map->inner_map_meta;
			/* transfer reg's id which is unique for every map_lookup_elem
			 * as UID of the inner map.
			 */
			if (map_value_has_timer(map->inner_map_meta))
				reg->map_uid = reg->id;
		} else if (map->map_type == BPF_MAP_TYPE_XSKMAP) {
			reg->type = PTR_TO_XDP_SOCK;
		} else if (map->map_type == BPF_MAP_TYPE_SOCKMAP ||
			   map->map_type == BPF_MAP_TYPE_SOCKHASH) {
			reg->type = PTR_TO_SOCKET;
		} else {
			reg->type = PTR_TO_MAP_VALUE;
		}
		break;
	}
	case PTR_TO_SOCKET_OR_NULL:
		reg->type = PTR_TO_SOCKET;
		break;
	case PTR_TO_SOCK_COMMON_OR_NULL:
		reg->type = PTR_TO_SOCK_COMMON;
		break;
	case PTR_TO_TCP_SOCK_OR_NULL:
		reg->type = PTR_TO_TCP_SOCK;
		break;
	case PTR_TO_BTF_ID_OR_NULL:
		reg->type = PTR_TO_BTF_ID;
		break;
	case PTR_TO_MEM_OR_NULL:
		reg->type = PTR_TO_MEM;
		break;
	case PTR_TO_RDONLY_BUF_OR_NULL:
		reg->type = PTR_TO_RDONLY_BUF;
		break;
	case PTR_TO_RDWR_BUF_OR_NULL:
		reg->type = PTR_TO_RDWR_BUF;
		break;
	default:
		WARN_ONCE(1, "unknown nullable register type");
	}
}