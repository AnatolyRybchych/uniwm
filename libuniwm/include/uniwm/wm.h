#ifndef WM_H
#define WM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <mc/data/str.h>
#include <mc/wm/wm.h>
#include <mc/wm/event.h>

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

typedef enum WM_WindowChange {
    WM_WINDOW_CREATED,
    WM_WINDOW_DESTROYED,
    WM_WINDOW_FOCUSED,
} WM_WindowChange;

typedef enum WM_TargetCapability {
    WM_TARGET_CAP_WINDOW_EVENTS = 1 << 0,
} WM_TargetCapability;

typedef uint32_t WM_TargetCapabilities;

typedef enum WM_UniwmEvent {
    WM_UNIWM_VDESKTOP_CHANGED,
    WM_UNIWM_WINDOW_CREATED,
    WM_UNIWM_WINDOW_DESTROYED,
    WM_UNIWM_WINDOW_FOCUSED,
    WM_UNIWM_EVENT_COUNT,
} WM_UniwmEvent;

struct WM {
    const WM_TargetInterface *target_interface;
    WM_Target *target;
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

WM_Error wm_window_watch_ensure(void);

WM_Error wm_input_ensure(void);
MC_WM *wm_input(void);
MC_WMEventType wm_uniwm_event_type(WM_UniwmEvent which);
WM_Error wm_resolve_window(uint64_t identity, MC_WindowRef **out);

WM_Error wm_key_combo_from_str(const char *spec, WM_KeyCombo *out);
WM_Error wm_suppress_key(WM *wm, const WM_KeyCombo *combo);
WM_Error wm_unsuppress_key(WM *wm, const WM_KeyCombo *combo);
WM_Error wm_bind_key(WM *wm, const WM_KeyCombo *combo, void (*cb)(void *ctx), void *ctx);
WM_Error wm_unbind_key(WM *wm, const WM_KeyCombo *combo, void **out_ctx);

bool wm_should_run(void);
void wm_process_event(const MC_WMEvent *event);
void wm_tick(void);

#endif
