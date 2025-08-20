static struct cmd_obj *cmd_hdl_filter(struct _adapter *padapter,
				      struct cmd_obj *pcmd)
{
	struct cmd_obj *pcmd_r;

	if (!pcmd)
		return pcmd;
	pcmd_r = NULL;

	switch (pcmd->cmdcode) {
	case GEN_CMD_CODE(_Read_MACREG):
		read_macreg_hdl(padapter, (u8 *)pcmd);
		pcmd_r = pcmd;
		break;
	case GEN_CMD_CODE(_Write_MACREG):
		write_macreg_hdl(padapter, (u8 *)pcmd);
		pcmd_r = pcmd;
		break;
	case GEN_CMD_CODE(_Read_BBREG):
		read_bbreg_hdl(padapter, (u8 *)pcmd);
		break;
	case GEN_CMD_CODE(_Write_BBREG):
		write_bbreg_hdl(padapter, (u8 *)pcmd);
		break;
	case GEN_CMD_CODE(_Read_RFREG):
		read_rfreg_hdl(padapter, (u8 *)pcmd);
		break;
	case GEN_CMD_CODE(_Write_RFREG):
		write_rfreg_hdl(padapter, (u8 *)pcmd);
		break;
	case GEN_CMD_CODE(_SetUsbSuspend):
		sys_suspend_hdl(padapter, (u8 *)pcmd);
		break;
	case GEN_CMD_CODE(_JoinBss):
		r8712_joinbss_reset(padapter);
		/* Before set JoinBss_CMD to FW, driver must ensure FW is in
		 * PS_MODE_ACTIVE. Directly write rpwm to radio on and assign
		 * new pwr_mode to Driver, instead of use workitem to change
		 * state.
		 */
		if (padapter->pwrctrlpriv.pwr_mode > PS_MODE_ACTIVE) {
			padapter->pwrctrlpriv.pwr_mode = PS_MODE_ACTIVE;
			mutex_lock(&padapter->pwrctrlpriv.mutex_lock);
			r8712_set_rpwm(padapter, PS_STATE_S4);
			mutex_unlock(&padapter->pwrctrlpriv.mutex_lock);
		}
		pcmd_r = pcmd;
		break;
	case _DRV_INT_CMD_:
		r871x_internal_cmd_hdl(padapter, pcmd->parmbuf);
		r8712_free_cmd_obj(pcmd);
		pcmd_r = NULL;
		break;
	default:
		pcmd_r = pcmd;
		break;
	}
	return pcmd_r; /* if returning pcmd_r == NULL, pcmd must be free. */
}
