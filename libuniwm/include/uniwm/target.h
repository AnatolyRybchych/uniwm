#ifndef WM_TARGET_H
#define WM_TARGET_H

#include <stdint.h>

#include <mc/data/str.h>

#include <uniwm/wm.h>
#include <uniwm/vdesktop.h>

typedef void (*WM_WindowHandleSink)(void *ctx, uint64_t handle);

struct WM_TargetInterface {
    WM_Error (*init)(MC_Alloc *alloc, WM_Target **target);
    void (*destroy)(WM_Target *target);

    WM_VDesktopSpan (*get_vdesk)(WM_Target *target);
    WM_Error (*vdesk_open)(WM_Target *target, WM_VDesktop *vdesk);
    WM_Error (*vdesk_create)(WM_Target *target, MC_Str name, WM_VDesktop **out);

    WM_VDesktop *(*vdesk_current)(WM_Target *target);

    MC_Str (*vdesk_name)(WM_Target *target, const WM_VDesktop *vdesk);

    WM_Error (*vdesk_windows)(WM_Target *target, const WM_VDesktop *vdesk, WM_WindowHandleSink sink, void *ctx);
    WM_Error (*vdesk_size)(WM_Target *target, const WM_VDesktop *vdesk, MC_Size2U *out);
    WM_Error (*vdesk_move_window)(WM_Target *target, const WM_VDesktop *vdesk, uint64_t handle);
};

#endif
