#include <stddef.h>
#include <string.h>

#include <uniwm/wm.h>
#include <uniwm/target.h>

WM_Error wm_init(WM *wm, const WM_TargetInterface *ti, MC_Alloc *alloc) {
    if (!wm || !ti || !alloc) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    wm->target_interface = ti;
    wm->target = NULL;
    wm->vdesktops = NULL;

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

WM_VDesktopSpan wm_vdesktops(WM *wm) {
    return wm->target_interface->get_vdesk(wm->target);
}

WM_Error wm_vdesktop_create(WM *wm, WM_VDesktop **out) {
    if (!out) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    return wm->target_interface->vdesk_new(wm->target, out);
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
