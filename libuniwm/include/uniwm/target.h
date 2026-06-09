#ifndef WM_TARGET_H
#define WM_TARGET_H

#include <mc/data/str.h>

#include <uniwm/wm.h>
#include <uniwm/vdesktop.h>
#include <uniwm/key.h>

struct WM_TargetInterface {
    WM_Error (*init)(MC_Alloc *alloc, WM_Target **target);
    void (*destroy)(WM_Target *target);

    WM_VDesktopSpan (*get_vdesk)(WM_Target *target);
    WM_Error (*vdesk_open)(WM_Target *target, WM_VDesktop *vdesk);

    WM_VDesktop *(*vdesk_current)(WM_Target *target);

    MC_Str (*vdesk_name)(WM_Target *target, const WM_VDesktop *vdesk);

    WM_Error (*suppress_key)(WM_Target *target, const WM_KeyCombo *combo);
    WM_Error (*unsuppress_key)(WM_Target *target, const WM_KeyCombo *combo);
    WM_Error (*run)(WM_Target *target);
};

#endif
