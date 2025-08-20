static void msm_pinctrl_setup_pm_reset(struct msm_pinctrl *pctrl)
{
	int i = 0;
	const struct msm_function *func = pctrl->soc->functions;

	for (; i <= pctrl->soc->nfunctions; i++)
		if (!strcmp(func[i].name, "ps_hold")) {
			pctrl->restart_nb.notifier_call = msm_ps_hold_restart;
			pctrl->restart_nb.priority = 128;
			if (register_restart_handler(&pctrl->restart_nb))
				dev_err(pctrl->dev,
					"failed to setup restart handler.\n");
			break;
		}
}
