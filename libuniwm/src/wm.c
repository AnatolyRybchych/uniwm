#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include <mc/wm/key.h>
#include <mc/wm/resolver.h>
#include <mc/win32_wm/wm.h>
#include <mc/data/alloc.h>

#include <uniwm/wm.h>
#include <uniwm/target.h>

WM_Error wm_init(WM *wm, const WM_TargetInterface *ti, MC_Alloc *alloc) {
    if (!wm || !ti || !alloc) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    wm->target_interface = ti;
    wm->target = NULL;
    wm->change_subs = NULL;

    return ti->init(alloc, &wm->target);
}

void wm_fini(WM *wm) {
    if (!wm || !wm->target_interface) {
        return;
    }

    if (wm->target) {
        wm->target_interface->destroy(wm->target);
    }

    MC_VECTOR_FREE(wm->change_subs);
    wm->change_subs = NULL;

    wm->target = NULL;
}

MC_Alloc *wm_allocator = &mc_alloc_malloc;

static const WM_TargetInterface *default_ti = NULL;
static WM process_wm;
static bool process_ready = false;

void wm_set_default(const WM_TargetInterface *ti) {
    default_ti = ti;

    mc_wm_resolver_register(mc_win32_wm_vtab);
}

WM *wm_process(void) {
    if (process_ready) {
        return &process_wm;
    }

    if (default_ti == NULL) {
        return NULL;
    }

    if (wm_init(&process_wm, default_ti, wm_allocator) != WM_ERROR_OK) {
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

WM_VDesktopSpan wm_vdesktops(WM *wm) {
    return wm->target_interface->get_vdesk(wm->target);
}

static void notify_vdesk_changed(WM *wm, WM_VDesktop *current, WM_VDesktopChangeSource source) {
    WM_VDesktopChangeSub *sub;
    MC_VECTOR_EACH(wm->change_subs, sub) {
        sub->cb(current, source, sub->ctx);
    }
}

WM_Error wm_vdesktop_switch(WM *wm, WM_VDesktop *vdesk) {
    if (!vdesk) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    WM_Error e = wm->target_interface->vdesk_open(wm->target, vdesk);
    if (e != WM_ERROR_OK) {
        return e;
    }

    notify_vdesk_changed(wm, vdesk, WM_VDESKTOP_CHANGE_MANAGED);

    return WM_ERROR_OK;
}

WM_Error wm_vdesktop_on_changed(WM *wm, WM_VDesktopChangeCb cb, void *ctx) {
    if (wm == NULL || cb == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    WM_VDesktopChangeSub sub = { .cb = cb, .ctx = ctx };

    WM_VDesktopChangeSubList *grown = MC_VECTOR_PUSHN(wm->change_subs, 1, (&sub));
    if (grown == NULL) {
        return WM_ERROR_OUT_OF_MEMORY;
    }
    wm->change_subs = grown;

    return WM_ERROR_OK;
}

WM_Error wm_vdesktop_create(WM *wm, MC_Str name, WM_VDesktop **out) {
    if (!wm || !out) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    if (!wm->target_interface->vdesk_create) {
        return WM_ERROR_NOT_IMPLEMENTED;
    }

    return wm->target_interface->vdesk_create(wm->target, name, out);
}

WM_VDesktop *wm_vdesktop_current(WM *wm) {
    return wm->target_interface->vdesk_current(wm->target);
}

MC_Str wm_vdesktop_name(WM *wm, const WM_VDesktop *vdesk) {
    return wm->target_interface->vdesk_name(wm->target, vdesk);
}

typedef struct WM_VDesktopWindowsCtx {
    MC_WM *mc_wm;
    MC_TargetWM *target;
    void (*visit)(MC_WindowRef *window, void *ctx);
    void *ctx;
} WM_VDesktopWindowsCtx;

static void resolve_window_handle(void *ctx, uint64_t handle) {
    WM_VDesktopWindowsCtx *c = ctx;

    uint64_t identity;
    if (mc_wm_win32_identity_from_hwnd(c->target, (HWND)(uintptr_t)handle, &identity) != MCE_OK) {
        return;
    }

    MC_WindowRef *window;
    if (mc_wm_resolve_window(c->mc_wm, identity, &window) != MCE_OK) {
        return;
    }

    c->visit(window, c->ctx);
}

WM_Error wm_vdesktop_windows(WM *wm, const WM_VDesktop *vdesk, void (*visit)(MC_WindowRef *window, void *ctx), void *ctx) {
    if (wm == NULL || vdesk == NULL || visit == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    if (wm->target_interface->vdesk_windows == NULL) {
        return WM_ERROR_NOT_IMPLEMENTED;
    }

    MC_WM *mc_wm;
    if (mc_wm_resolve(&mc_wm) != MCE_OK) {
        return WM_ERROR_UNKNOWN;
    }

    WM_VDesktopWindowsCtx c = {
        .mc_wm = mc_wm,
        .target = mc_wm_get_target(mc_wm),
        .visit = visit,
        .ctx = ctx,
    };

    return wm->target_interface->vdesk_windows(wm->target, vdesk, resolve_window_handle, &c);
}

WM_Error wm_vdesktop_size(WM *wm, const WM_VDesktop *vdesk, MC_Size2U *out) {
    if (wm == NULL || vdesk == NULL || out == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    if (wm->target_interface->vdesk_size == NULL) {
        return WM_ERROR_NOT_IMPLEMENTED;
    }

    return wm->target_interface->vdesk_size(wm->target, vdesk, out);
}
