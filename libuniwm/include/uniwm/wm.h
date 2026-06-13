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

typedef enum WM_WindowChange {
    WM_WINDOW_CREATED,
    WM_WINDOW_DESTROYED,
} WM_WindowChange;

typedef enum WM_EventType {
    WM_EVENT_VDESKTOP_CHANGED,
    WM_EVENT_WINDOW_CREATED,
    WM_EVENT_WINDOW_DESTROYED,
} WM_EventType;

typedef struct WM_Event {
    WM_EventType type;
    union {
        struct {
            WM_VDesktop *desktop;
            WM_VDesktopChangeSource source;
        } vdesktop_changed;

        struct {
            MC_WindowRef *window;
        } window_created;

        struct {
            MC_WindowRef *window;
        } window_destroyed;
    } as;
} WM_Event;

typedef struct WM_Subscription WM_Subscription;

struct WM {
    const WM_TargetInterface *target_interface;
    WM_Target *target;
    WM_Subscription *subs;
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
WM_Error wm_subscribe(WM *wm, WM_EventType type, void (*cb)(const WM_Event *event, void *user_data), void *user_data, WM_Subscription **out);
void wm_unsubscribe(WM_Subscription *sub);
void wm_dispatch_event(WM *wm, const WM_Event *event);
bool wm_has_subscribers(WM *wm, WM_EventType type);

WM_Error wm_watch_window_changed(WM *wm, void (*sink)(void *ctx, uint64_t handle, WM_WindowChange change), void *ctx);
WM_Error wm_window_watch_ensure(void);

WM_Error wm_input_ensure(void);

WM_Error wm_key_combo_from_str(const char *spec, WM_KeyCombo *out);
WM_Error wm_suppress_key(WM *wm, const WM_KeyCombo *combo);
WM_Error wm_unsuppress_key(WM *wm, const WM_KeyCombo *combo);
WM_Error wm_bind_key(WM *wm, const WM_KeyCombo *combo, void (*cb)(void *ctx), void *ctx);
WM_Error wm_unbind_key(WM *wm, const WM_KeyCombo *combo, void **out_ctx);
WM_Error wm_run(void);

#endif
