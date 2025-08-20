static inline int nvme_reset_subsystem(struct nvme_ctrl *ctrl)
{
	if (!ctrl->subsystem)
		return -ENOTTY;
	return ctrl->ops->reg_write32(ctrl, NVME_REG_NSSR, 0x4E564D65);
}
