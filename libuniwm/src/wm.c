#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include <mc/wm/key.h>

#include <uniwm/wm.h>
#include <uniwm/target.h>

WM_Error wm_init(WM *wm, const WM_TargetInterface *ti, MC_Alloc *alloc) {
    if (!wm || !ti || !alloc) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    wm->target_interface = ti;
    wm->target = NULL;

    return ti->init(alloc, &wm->target);
}

void wm_fini(WM *wm) {
    if (!wm || !wm->target_interface) {
        return;
    }

    if (wm->target) {
        wm->target_interface->destroy(wm->target);
    }

    wm->target = NULL;
}

static const WM_TargetInterface *default_ti = NULL;
static MC_Alloc *default_alloc = NULL;
static WM process_wm;
static bool process_ready = false;

void wm_set_default(const WM_TargetInterface *ti, MC_Alloc *alloc) {
    default_ti = ti;
    default_alloc = alloc;
}

WM *wm_process(void) {
    if (process_ready) {
        return &process_wm;
    }

    if (default_ti == NULL || default_alloc == NULL) {
        return NULL;
    }

    if (wm_init(&process_wm, default_ti, default_alloc) != WM_ERROR_OK) {
        return NULL;
    }

    process_ready = true;
    return &process_wm;
}

void wm_process_fini(void) {
    if (!process_ready) {
        return;
    }

    wm_fini(&process_wm);
    process_ready = false;
}

const WM_TargetInterface *wm_default_target(void) {
    return default_ti;
}

WM_Error wm_key_combo_from_str(const char *spec, WM_KeyCombo *out) {
    if (spec == NULL || out == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    MC_Str s = mc_strc(spec);

    WM_KeyCombo combo = {0};

    const char *p = s.beg;
    for (;;) {
        const char *plus = p;
        while (plus < s.end && *plus != '+') {
            plus++;
        }

        MC_Key key = mc_key_from_str(MC_STR(p, plus));
        if (key == MC_KEY_UNKNOWN) {
            return WM_ERROR_INVALID_ARGUMENT;
        }

        if (combo.count >= WM_KEY_COMBO_MAX) {
            return WM_ERROR_INVALID_ARGUMENT;
        }
        combo.keys[combo.count++] = key;

        if (plus >= s.end) {
            break;
        }
        p = plus + 1;
    }

    *out = combo;
    return WM_ERROR_OK;
}

WM_Error wm_suppress_key(WM *wm, const WM_KeyCombo *combo) {
    if (!wm || !wm->target_interface || !combo) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    if (!wm->target_interface->suppress_key) {
        return WM_ERROR_NOT_IMPLEMENTED;
    }

    return wm->target_interface->suppress_key(wm->target, combo);
}

WM_Error wm_unsuppress_key(WM *wm, const WM_KeyCombo *combo) {
    if (!wm || !wm->target_interface || !combo) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    if (!wm->target_interface->unsuppress_key) {
        return WM_ERROR_NOT_IMPLEMENTED;
    }

    return wm->target_interface->unsuppress_key(wm->target, combo);
}

WM_Error wm_run(void) {
    if (!process_ready || !process_wm.target_interface->run) {
        return WM_ERROR_OK;
    }

    return process_wm.target_interface->run(process_wm.target);
}

WM_VDesktopSpan wm_vdesktops(WM *wm) {
    return wm->target_interface->get_vdesk(wm->target);
}

WM_Error wm_vdesktop_switch(WM *wm, WM_VDesktop *vdesk) {
    if (!vdesk) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    return wm->target_interface->vdesk_open(wm->target, vdesk);
}

WM_VDesktop *wm_vdesktop_current(WM *wm) {
    return wm->target_interface->vdesk_current(wm->target);
}

MC_Str wm_vdesktop_name(WM *wm, const WM_VDesktop *vdesk) {
    return wm->target_interface->vdesk_name(wm->target, vdesk);
}
