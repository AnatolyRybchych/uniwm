#ifndef WM_H
#define WM_H

#include <stddef.h>

#include <mc/data/str.h>

#include <uniwm/vdesktop.h>
#include <uniwm/key.h>

typedef struct WM WM;
typedef struct WM_TargetInterface WM_TargetInterface;
typedef struct WM_Target WM_Target;
typedef struct MC_Alloc MC_Alloc;

typedef enum WM_Error {
    WM_ERROR_OK = 0,
    WM_ERROR_UNKNOWN = 1,
    WM_ERROR_INVALID_ARGUMENT = 2,
    WM_ERROR_OUT_OF_MEMORY = 3,
    WM_ERROR_NOT_IMPLEMENTED = 4,
} WM_Error;

struct WM {
    const WM_TargetInterface *target_interface;
    WM_Target *target;
};

WM_Error wm_init(WM *wm, const WM_TargetInterface *ti, MC_Alloc *alloc);
void wm_fini(WM *wm);

void wm_set_default(const WM_TargetInterface *ti, MC_Alloc *alloc);
WM *wm_process(void);
void wm_process_fini(void);

WM_VDesktopSpan wm_vdesktops(WM *wm);
WM_Error wm_vdesktop_switch(WM *wm, WM_VDesktop *vdesk);
WM_VDesktop *wm_vdesktop_current(WM *wm);
MC_Str wm_vdesktop_name(WM *wm, const WM_VDesktop *vdesk);

const WM_TargetInterface *wm_default_target(void);
WM_Error wm_key_combo_from_str(const char *spec, WM_KeyCombo *out);
WM_Error wm_suppress_key(WM *wm, const WM_KeyCombo *combo);
WM_Error wm_unsuppress_key(WM *wm, const WM_KeyCombo *combo);
WM_Error wm_run(void);

#endif
