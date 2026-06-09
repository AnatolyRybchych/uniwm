#include <stddef.h>
#include <stdbool.h>
#include <string.h>

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
