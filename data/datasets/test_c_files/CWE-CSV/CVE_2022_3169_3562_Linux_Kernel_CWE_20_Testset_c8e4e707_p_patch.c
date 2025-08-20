static inline int nvme_reset_subsystem(struct nvme_ctrl *ctrl)
{
	int ret;

	if (!ctrl->subsystem)
		return -ENOTTY;
	if (!nvme_wait_reset(ctrl))
		return -EBUSY;

	ret = ctrl->ops->reg_write32(ctrl, NVME_REG_NSSR, 0x4E564D65);
	if (ret)
		return ret;

	return nvme_try_sched_reset(ctrl);
}
