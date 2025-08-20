static int dpu_crtc_atomic_check(struct drm_crtc *crtc,
		struct drm_atomic_state *state)
{
	struct drm_crtc_state *crtc_state = drm_atomic_get_new_crtc_state(state,
									  crtc);
	struct dpu_crtc *dpu_crtc = to_dpu_crtc(crtc);
	struct dpu_crtc_state *cstate = to_dpu_crtc_state(crtc_state);
	struct plane_state *pstates;

	const struct drm_plane_state *pstate;
	struct drm_plane *plane;
	struct drm_display_mode *mode;

	int cnt = 0, rc = 0, mixer_width = 0, i, z_pos;

	struct dpu_multirect_plane_states multirect_plane[DPU_STAGE_MAX * 2];
	int multirect_count = 0;
	const struct drm_plane_state *pipe_staged[SSPP_MAX];
	int left_zpos_cnt = 0, right_zpos_cnt = 0;
	struct drm_rect crtc_rect = { 0 };
	bool needs_dirtyfb = dpu_crtc_needs_dirtyfb(crtc_state);

	pstates = kzalloc(sizeof(*pstates) * DPU_STAGE_MAX * 4, GFP_KERNEL);
	if (!pstates)
		return -ENOMEM;

	if (!crtc_state->enable || !crtc_state->active) {
		DRM_DEBUG_ATOMIC("crtc%d -> enable %d, active %d, skip atomic_check\n",
				crtc->base.id, crtc_state->enable,
				crtc_state->active);
		memset(&cstate->new_perf, 0, sizeof(cstate->new_perf));
		goto end;
	}

	mode = &crtc_state->adjusted_mode;
	DRM_DEBUG_ATOMIC("%s: check\n", dpu_crtc->name);

	/* force a full mode set if active state changed */
	if (crtc_state->active_changed)
		crtc_state->mode_changed = true;

	memset(pipe_staged, 0, sizeof(pipe_staged));

	if (cstate->num_mixers) {
		mixer_width = mode->hdisplay / cstate->num_mixers;

		_dpu_crtc_setup_lm_bounds(crtc, crtc_state);
	}

	crtc_rect.x2 = mode->hdisplay;
	crtc_rect.y2 = mode->vdisplay;

	 /* get plane state for all drm planes associated with crtc state */
	drm_atomic_crtc_state_for_each_plane_state(plane, pstate, crtc_state) {
		struct dpu_plane_state *dpu_pstate = to_dpu_plane_state(pstate);
		struct drm_rect dst, clip = crtc_rect;

		if (IS_ERR_OR_NULL(pstate)) {
			rc = PTR_ERR(pstate);
			DPU_ERROR("%s: failed to get plane%d state, %d\n",
					dpu_crtc->name, plane->base.id, rc);
			goto end;
		}
		if (cnt >= DPU_STAGE_MAX * 4)
			continue;

		if (!pstate->visible)
			continue;

		pstates[cnt].dpu_pstate = dpu_pstate;
		pstates[cnt].drm_pstate = pstate;
		pstates[cnt].stage = pstate->normalized_zpos;
		pstates[cnt].pipe_id = dpu_plane_pipe(plane);

		dpu_pstate->needs_dirtyfb = needs_dirtyfb;

		if (pipe_staged[pstates[cnt].pipe_id]) {
			multirect_plane[multirect_count].r0 =
				pipe_staged[pstates[cnt].pipe_id];
			multirect_plane[multirect_count].r1 = pstate;
			multirect_count++;

			pipe_staged[pstates[cnt].pipe_id] = NULL;
		} else {
			pipe_staged[pstates[cnt].pipe_id] = pstate;
		}

		cnt++;

		dst = drm_plane_state_dest(pstate);
		if (!drm_rect_intersect(&clip, &dst)) {
			DPU_ERROR("invalid vertical/horizontal destination\n");
			DPU_ERROR("display: " DRM_RECT_FMT " plane: "
				  DRM_RECT_FMT "\n", DRM_RECT_ARG(&crtc_rect),
				  DRM_RECT_ARG(&dst));
			rc = -E2BIG;
			goto end;
		}
	}

	for (i = 1; i < SSPP_MAX; i++) {
		if (pipe_staged[i])
			dpu_plane_clear_multirect(pipe_staged[i]);
	}

	z_pos = -1;
	for (i = 0; i < cnt; i++) {
		/* reset counts at every new blend stage */
		if (pstates[i].stage != z_pos) {
			left_zpos_cnt = 0;
			right_zpos_cnt = 0;
			z_pos = pstates[i].stage;
		}

		/* verify z_pos setting before using it */
		if (z_pos >= DPU_STAGE_MAX - DPU_STAGE_0) {
			DPU_ERROR("> %d plane stages assigned\n",
					DPU_STAGE_MAX - DPU_STAGE_0);
			rc = -EINVAL;
			goto end;
		} else if (pstates[i].drm_pstate->crtc_x < mixer_width) {
			if (left_zpos_cnt == 2) {
				DPU_ERROR("> 2 planes @ stage %d on left\n",
					z_pos);
				rc = -EINVAL;
				goto end;
			}
			left_zpos_cnt++;

		} else {
			if (right_zpos_cnt == 2) {
				DPU_ERROR("> 2 planes @ stage %d on right\n",
					z_pos);
				rc = -EINVAL;
				goto end;
			}
			right_zpos_cnt++;
		}

		pstates[i].dpu_pstate->stage = z_pos + DPU_STAGE_0;
		DRM_DEBUG_ATOMIC("%s: zpos %d\n", dpu_crtc->name, z_pos);
	}

	for (i = 0; i < multirect_count; i++) {
		if (dpu_plane_validate_multirect_v2(&multirect_plane[i])) {
			DPU_ERROR(
			"multirect validation failed for planes (%d - %d)\n",
					multirect_plane[i].r0->plane->base.id,
					multirect_plane[i].r1->plane->base.id);
			rc = -EINVAL;
			goto end;
		}
	}

	atomic_inc(&_dpu_crtc_get_kms(crtc)->bandwidth_ref);

	rc = dpu_core_perf_crtc_check(crtc, crtc_state);
	if (rc) {
		DPU_ERROR("crtc%d failed performance check %d\n",
				crtc->base.id, rc);
		goto end;
	}

	/* validate source split:
	 * use pstates sorted by stage to check planes on same stage
	 * we assume that all pipes are in source split so its valid to compare
	 * without taking into account left/right mixer placement
	 */
	for (i = 1; i < cnt; i++) {
		struct plane_state *prv_pstate, *cur_pstate;
		struct drm_rect left_rect, right_rect;
		int32_t left_pid, right_pid;
		int32_t stage;

		prv_pstate = &pstates[i - 1];
		cur_pstate = &pstates[i];
		if (prv_pstate->stage != cur_pstate->stage)
			continue;

		stage = cur_pstate->stage;

		left_pid = prv_pstate->dpu_pstate->base.plane->base.id;
		left_rect = drm_plane_state_dest(prv_pstate->drm_pstate);

		right_pid = cur_pstate->dpu_pstate->base.plane->base.id;
		right_rect = drm_plane_state_dest(cur_pstate->drm_pstate);

		if (right_rect.x1 < left_rect.x1) {
			swap(left_pid, right_pid);
			swap(left_rect, right_rect);
		}

		/**
		 * - planes are enumerated in pipe-priority order such that
		 *   planes with lower drm_id must be left-most in a shared
		 *   blend-stage when using source split.
		 * - planes in source split must be contiguous in width
		 * - planes in source split must have same dest yoff and height
		 */
		if (right_pid < left_pid) {
			DPU_ERROR(
				"invalid src split cfg. priority mismatch. stage: %d left: %d right: %d\n",
				stage, left_pid, right_pid);
			rc = -EINVAL;
			goto end;
		} else if (right_rect.x1 != drm_rect_width(&left_rect)) {
			DPU_ERROR("non-contiguous coordinates for src split. "
				  "stage: %d left: " DRM_RECT_FMT " right: "
				  DRM_RECT_FMT "\n", stage,
				  DRM_RECT_ARG(&left_rect),
				  DRM_RECT_ARG(&right_rect));
			rc = -EINVAL;
			goto end;
		} else if (left_rect.y1 != right_rect.y1 ||
			   drm_rect_height(&left_rect) != drm_rect_height(&right_rect)) {
			DPU_ERROR("source split at stage: %d. invalid "
				  "yoff/height: left: " DRM_RECT_FMT " right: "
				  DRM_RECT_FMT "\n", stage,
				  DRM_RECT_ARG(&left_rect),
				  DRM_RECT_ARG(&right_rect));
			rc = -EINVAL;
			goto end;
		}
	}

end:
	kfree(pstates);
	return rc;
}
