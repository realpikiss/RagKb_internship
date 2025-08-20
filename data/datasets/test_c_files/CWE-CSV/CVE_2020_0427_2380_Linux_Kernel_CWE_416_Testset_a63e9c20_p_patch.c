static int dt_remember_or_free_map(struct pinctrl *p, const char *statename,
				   struct pinctrl_dev *pctldev,
				   struct pinctrl_map *map, unsigned num_maps)
{
	int i;
	struct pinctrl_dt_map *dt_map;

	/* Initialize common mapping table entry fields */
	for (i = 0; i < num_maps; i++) {
		const char *devname;

		devname = kstrdup_const(dev_name(p->dev), GFP_KERNEL);
		if (!devname)
			goto err_free_map;

		map[i].dev_name = devname;
		map[i].name = statename;
		if (pctldev)
			map[i].ctrl_dev_name = dev_name(pctldev->dev);
	}

	/* Remember the converted mapping table entries */
	dt_map = kzalloc(sizeof(*dt_map), GFP_KERNEL);
	if (!dt_map)
		goto err_free_map;

	dt_map->pctldev = pctldev;
	dt_map->map = map;
	dt_map->num_maps = num_maps;
	list_add_tail(&dt_map->node, &p->dt_maps);

	return pinctrl_register_map(map, num_maps, false);

err_free_map:
	dt_free_map(pctldev, map, num_maps);
	return -ENOMEM;
}
