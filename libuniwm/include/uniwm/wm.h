#ifndef WM_H
#define WM_H

#include <stddef.h>

#include <mc/data/str.h>
#include <mc/wm/wm.h>

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

typedef enum WM_VDesktopChangeSource {
    WM_VDESKTOP_CHANGE_MANAGED,
    WM_VDESKTOP_CHANGE_EXTERNAL,
} WM_VDesktopChangeSource;

typedef void (*WM_VDesktopChangeCb)(WM_VDesktop *current, WM_VDesktopChangeSource source, void *ctx);

typedef struct WM_VDesktopChangeSub {
    WM_VDesktopChangeCb cb;
    void *ctx;
} WM_VDesktopChangeSub;

MC_DEFINE_VECTOR(WM_VDesktopChangeSubList, WM_VDesktopChangeSub);

struct WM {
    const WM_TargetInterface *target_interface;
    WM_Target *target;
    WM_VDesktopChangeSubList *change_subs;
};

WM_Error wm_init(WM *wm, const WM_TargetInterface *ti, MC_Alloc *alloc);
void wm_fini(WM *wm);

extern MC_Alloc *wm_allocator;

void wm_set_default(const WM_TargetInterface *ti);
WM *wm_process(void);
void wm_process_fini(void);

WM_VDesktopSpan wm_vdesktops(WM *wm);
WM_Error wm_vdesktop_switch(WM *wm, WM_VDesktop *vdesk);
WM_Error wm_vdesktop_create(WM *wm, MC_Str name, WM_VDesktop **out);
WM_VDesktop *wm_vdesktop_current(WM *wm);
MC_Str wm_vdesktop_name(WM *wm, const WM_VDesktop *vdesk);
WM_Error wm_vdesktop_windows(WM *wm, const WM_VDesktop *vdesk, void (*visit)(MC_WindowRef *window, void *ctx), void *ctx);
WM_Error wm_vdesktop_size(WM *wm, const WM_VDesktop *vdesk, MC_Size2U *out);
WM_Error wm_vdesktop_move_window(WM *wm, const WM_VDesktop *vdesk, uint64_t handle);
WM_Error wm_vdesktop_on_changed(WM *wm, WM_VDesktopChangeCb cb, void *ctx);

WM_Error wm_input_ensure(void);

WM_Error wm_key_combo_from_str(const char *spec, WM_KeyCombo *out);
WM_Error wm_suppress_key(WM *wm, const WM_KeyCombo *combo);
WM_Error wm_unsuppress_key(WM *wm, const WM_KeyCombo *combo);
WM_Error wm_bind_key(WM *wm, const WM_KeyCombo *combo, void (*cb)(void *ctx), void *ctx);
WM_Error wm_unbind_key(WM *wm, const WM_KeyCombo *combo, void **out_ctx);
WM_Error wm_run(void);

#endif
