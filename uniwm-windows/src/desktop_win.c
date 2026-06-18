#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <windows.h>
#include <objbase.h>
#include <servprov.h>
#include <objectarray.h>
#include <winstring.h>
#include <dwmapi.h>

#include <mc/data/alloc.h>
#include <mc/data/vector.h>
#include <mc/data/str.h>
#include <mc/data/string.h>

#include <uniwm/wm.h>
#include <uniwm/target.h>
#include <uniwm-windows/target.h>

typedef struct IVirtualDesktop IVirtualDesktop;

typedef struct IVirtualDesktopVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IVirtualDesktop *this, REFIID riid, void **ppv);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IVirtualDesktop *this);
    ULONG   (STDMETHODCALLTYPE *Release)(IVirtualDesktop *this);
    HRESULT (STDMETHODCALLTYPE *IsViewVisible)(IVirtualDesktop *this, IUnknown *view, int *visible);
    HRESULT (STDMETHODCALLTYPE *GetID)(IVirtualDesktop *this, GUID *id);
    HRESULT (STDMETHODCALLTYPE *GetName)(IVirtualDesktop *this, HSTRING *name);
    HRESULT (STDMETHODCALLTYPE *GetWallpaperPath)(IVirtualDesktop *this, HSTRING *path);
} IVirtualDesktopVtbl;

struct IVirtualDesktop {
    const IVirtualDesktopVtbl *lpVtbl;
};

typedef struct IVirtualDesktopManagerInternal IVirtualDesktopManagerInternal;

typedef struct IVirtualDesktopManagerInternalVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IVirtualDesktopManagerInternal *this, REFIID riid, void **ppv);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IVirtualDesktopManagerInternal *this);
    ULONG   (STDMETHODCALLTYPE *Release)(IVirtualDesktopManagerInternal *this);

    HRESULT (STDMETHODCALLTYPE *GetCount)(IVirtualDesktopManagerInternal *this, UINT *count);
    HRESULT (STDMETHODCALLTYPE *MoveViewToDesktop)(IVirtualDesktopManagerInternal *this, IUnknown *view, IVirtualDesktop *desktop);
    HRESULT (STDMETHODCALLTYPE *CanViewMoveDesktops)(IVirtualDesktopManagerInternal *this, IUnknown *view, int *can_move);
    HRESULT (STDMETHODCALLTYPE *GetCurrentDesktop)(IVirtualDesktopManagerInternal *this, IVirtualDesktop **desktop);
    HRESULT (STDMETHODCALLTYPE *GetDesktops)(IVirtualDesktopManagerInternal *this, IObjectArray **desktops);
    HRESULT (STDMETHODCALLTYPE *GetAdjacentDesktop)(IVirtualDesktopManagerInternal *this, IVirtualDesktop *from, int direction, IVirtualDesktop **desktop);
    HRESULT (STDMETHODCALLTYPE *SwitchDesktop)(IVirtualDesktopManagerInternal *this, IVirtualDesktop *desktop);
    HRESULT (STDMETHODCALLTYPE *SwitchDesktopAndMoveForegroundView)(IVirtualDesktopManagerInternal *this, IVirtualDesktop *desktop);
    HRESULT (STDMETHODCALLTYPE *CreateDesktop)(IVirtualDesktopManagerInternal *this, IVirtualDesktop **desktop);
    HRESULT (STDMETHODCALLTYPE *MoveDesktop)(IVirtualDesktopManagerInternal *this, IVirtualDesktop *desktop, UINT index);
    HRESULT (STDMETHODCALLTYPE *RemoveDesktop)(IVirtualDesktopManagerInternal *this, IVirtualDesktop *remove, IVirtualDesktop *fallback);
    HRESULT (STDMETHODCALLTYPE *FindDesktop)(IVirtualDesktopManagerInternal *this, const GUID *id, IVirtualDesktop **desktop);
    HRESULT (STDMETHODCALLTYPE *GetDesktopSwitchIncludeExcludeViews)(IVirtualDesktopManagerInternal *this, IVirtualDesktop *desktop, IObjectArray **include, IObjectArray **exclude);
    HRESULT (STDMETHODCALLTYPE *SetDesktopName)(IVirtualDesktopManagerInternal *this, IVirtualDesktop *desktop, HSTRING name);
} IVirtualDesktopManagerInternalVtbl;

struct IVirtualDesktopManagerInternal {
    const IVirtualDesktopManagerInternalVtbl *lpVtbl;
};

static const CLSID WM_CLSID_ImmersiveShell = {
    0xC2F03A33, 0x21F5, 0x47FA,
    { 0xB4, 0xBB, 0x15, 0x63, 0x62, 0xA2, 0xF2, 0x39 } };

static const GUID WM_CLSID_VirtualDesktopManagerInternal = {
    0xC5E0CDCA, 0x7B6E, 0x41B2,
    { 0x9F, 0xC4, 0xD9, 0x39, 0x75, 0xCC, 0x46, 0x7B } };

static const IID WM_IID_IVirtualDesktopManagerInternal = {
    0x53F5CA0B, 0x158F, 0x4124,
    { 0x90, 0x0C, 0x05, 0x71, 0x58, 0x06, 0x0B, 0x27 } };

static const IID WM_IID_IVirtualDesktop = {
    0x3F07F4BE, 0xB107, 0x441A,
    { 0xAF, 0x0F, 0x39, 0xD8, 0x25, 0x29, 0x07, 0x2C } };

typedef struct IVirtualDesktopManager IVirtualDesktopManager;

typedef struct IVirtualDesktopManagerVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IVirtualDesktopManager *this, REFIID riid, void **ppv);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IVirtualDesktopManager *this);
    ULONG   (STDMETHODCALLTYPE *Release)(IVirtualDesktopManager *this);
    HRESULT (STDMETHODCALLTYPE *IsWindowOnCurrentVirtualDesktop)(IVirtualDesktopManager *this, HWND hwnd, BOOL *on_current);
    HRESULT (STDMETHODCALLTYPE *GetWindowDesktopId)(IVirtualDesktopManager *this, HWND hwnd, GUID *id);
    HRESULT (STDMETHODCALLTYPE *MoveWindowToDesktop)(IVirtualDesktopManager *this, HWND hwnd, const GUID *id);
} IVirtualDesktopManagerVtbl;

struct IVirtualDesktopManager {
    const IVirtualDesktopManagerVtbl *lpVtbl;
};

static const CLSID WM_CLSID_VirtualDesktopManager = {
    0xAA509086, 0x5CA9, 0x4C25,
    { 0x8F, 0x95, 0x58, 0x9D, 0x3C, 0x07, 0xB4, 0x8A } };

static const IID WM_IID_IVirtualDesktopManager = {
    0xA5CD92FF, 0x29BE, 0x454C,
    { 0x8D, 0x04, 0xD8, 0x28, 0x79, 0xFB, 0x3F, 0x1B } };

typedef struct IApplicationViewCollection IApplicationViewCollection;

typedef struct IApplicationViewCollectionVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IApplicationViewCollection *this, REFIID riid, void **ppv);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IApplicationViewCollection *this);
    ULONG   (STDMETHODCALLTYPE *Release)(IApplicationViewCollection *this);
    HRESULT (STDMETHODCALLTYPE *GetViews)(IApplicationViewCollection *this, IObjectArray **out);
    HRESULT (STDMETHODCALLTYPE *GetViewsByZOrder)(IApplicationViewCollection *this, IObjectArray **out);
    HRESULT (STDMETHODCALLTYPE *GetViewsByAppUserModelId)(IApplicationViewCollection *this, const wchar_t *id, IObjectArray **out);
    HRESULT (STDMETHODCALLTYPE *GetViewForHwnd)(IApplicationViewCollection *this, HWND hwnd, IUnknown **view);
} IApplicationViewCollectionVtbl;

struct IApplicationViewCollection {
    const IApplicationViewCollectionVtbl *lpVtbl;
};

static const IID WM_IID_IApplicationViewCollection = {
    0x1841C6D7, 0x4F9D, 0x42C0,
    { 0xAF, 0x41, 0x87, 0x47, 0x53, 0x8F, 0x10, 0xE5 } };

struct WM_VDesktop {
    IVirtualDesktop *iface;
    GUID id;
    MC_String *name;
    bool valid;
};

struct WM_Target {
    MC_Alloc *alloc;
    IServiceProvider *sp;
    IVirtualDesktopManagerInternal *vdmi;
    IVirtualDesktopManager *vdm;
    IApplicationViewCollection *avc;

    WM_VDesktopList *items;
};

static int guid_eq(const GUID *a, const GUID *b) {
    return memcmp(a, b, sizeof(GUID)) == 0;
}

static int hstring_to_utf8(HSTRING hs, char *buf, size_t cap) {
    UINT32 len = 0;
    const wchar_t *raw = WindowsGetStringRawBuffer(hs, &len);
    if (!raw || len == 0) {
        return 0;
    }

    int n = WideCharToMultiByte(CP_UTF8, 0, raw, (int)len, buf, (int)cap - 1, NULL, NULL);
    if (n <= 0) {
        return 0;
    }

    buf[n] = '\0';
    return n;
}

static WM_Error utf8_to_hstring(MC_Str s, HSTRING *out) {
    int len = (int)MC_STR_LEN(s);
    if (len <= 0) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    wchar_t wbuf[128];
    int n = MultiByteToWideChar(CP_UTF8, 0, s.beg, len, wbuf, (int)(sizeof(wbuf) / sizeof(wbuf[0])));
    if (n <= 0) {
        return WM_ERROR_UNKNOWN;
    }

    if (FAILED(WindowsCreateString(wbuf, (UINT32)n, out))) {
        return WM_ERROR_UNKNOWN;
    }

    return WM_ERROR_OK;
}

static WM_Error fetch_name(WM_Target *t, IVirtualDesktop *iface, size_t ordinal, MC_String **out) {
    char buf[128];
    HSTRING hs = NULL;
    if (!(SUCCEEDED(iface->lpVtbl->GetName(iface, &hs)) && hstring_to_utf8(hs, buf, sizeof(buf)))) {
        snprintf(buf, sizeof(buf), "Desktop %zu", ordinal + 1);
    }
    if (hs) {
        WindowsDeleteString(hs);
    }

    if (mc_string(t->alloc, out, mc_strc(buf)) != MCE_OK) {
        return WM_ERROR_OUT_OF_MEMORY;
    }

    return WM_ERROR_OK;
}

static WM_VDesktop *wrap_desktop(WM_Target *t, IVirtualDesktop *iface, size_t ordinal) {
    WM_VDesktop *d = NULL;
    if (mc_alloc(t->alloc, sizeof(*d), (void **)&d) != MCE_OK) {
        return NULL;
    }

    d->iface = iface;
    d->name = NULL;
    d->valid = true;
    memset(&d->id, 0, sizeof(d->id));
    iface->lpVtbl->GetID(iface, &d->id);

    if (fetch_name(t, iface, ordinal, &d->name) != WM_ERROR_OK) {
        mc_free(t->alloc, d);
        return NULL;
    }

    return d;
}

static WM_Error desktop_update(WM_Target *t, WM_VDesktop *d, IVirtualDesktop *iface, size_t ordinal) {
    MC_String *name = NULL;
    if (fetch_name(t, iface, ordinal, &name) != WM_ERROR_OK) {
        return WM_ERROR_OUT_OF_MEMORY;
    }

    d->iface->lpVtbl->Release(d->iface);
    d->iface = iface;
    mc_free(t->alloc, d->name);
    d->name = name;

    return WM_ERROR_OK;
}

static void destroy_desktop(WM_Target *t, WM_VDesktop *d) {
    if (!d) {
        return;
    }

    if (d->iface) {
        d->iface->lpVtbl->Release(d->iface);
    }

    mc_free(t->alloc, d->name);
    mc_free(t->alloc, d);
}

static void reset_cache(WM_Target *t) {
    WM_VDesktop **it;
    MC_VECTOR_EACH(t->items, it) {
        (*it)->valid = false;
    }
}

static WM_Error push_desktop(WM_Target *t, WM_VDesktop *d) {
    WM_VDesktopList *grown = MC_VECTOR_PUSHN(t->items, 1, &d);
    if (!grown) {
        return WM_ERROR_OUT_OF_MEMORY;
    }
    t->items = grown;

    return WM_ERROR_OK;
}

static WM_VDesktop *find_desktop(WM_Target *t, const GUID *id) {
    WM_VDesktop **it;
    MC_VECTOR_EACH(t->items, it) {
        if (guid_eq(&(*it)->id, id)) {
            return *it;
        }
    }

    return NULL;
}

static WM_Error refresh(WM_Target *t) {
    IObjectArray *arr = NULL;
    if (FAILED(t->vdmi->lpVtbl->GetDesktops(t->vdmi, &arr)) || !arr) {
        return WM_ERROR_UNKNOWN;
    }

    UINT n = 0;
    arr->lpVtbl->GetCount(arr, &n);

    reset_cache(t);

    for (UINT i = 0; i < n; i++) {
        IVirtualDesktop *vd = NULL;
        if (FAILED(arr->lpVtbl->GetAt(arr, i, &WM_IID_IVirtualDesktop, (void **)&vd)) || !vd) {
            continue;
        }

        GUID id;
        vd->lpVtbl->GetID(vd, &id);

        WM_VDesktop *existing = find_desktop(t, &id);
        if (existing) {
            if (desktop_update(t, existing, vd, i) != WM_ERROR_OK) {
                vd->lpVtbl->Release(vd);
                arr->lpVtbl->Release(arr);
                return WM_ERROR_OUT_OF_MEMORY;
            }
            existing->valid = true;
        } else {
            WM_VDesktop *d = wrap_desktop(t, vd, i);
            if (!d) {
                vd->lpVtbl->Release(vd);
                arr->lpVtbl->Release(arr);
                return WM_ERROR_OUT_OF_MEMORY;
            }

            if (push_desktop(t, d) != WM_ERROR_OK) {
                destroy_desktop(t, d);
                arr->lpVtbl->Release(arr);
                return WM_ERROR_OUT_OF_MEMORY;
            }
        }
    }

    arr->lpVtbl->Release(arr);
    return WM_ERROR_OK;
}

static WM_Error win_init(MC_Alloc *alloc, WM_Target **out) {
    WM_Target *t = NULL;
    if (mc_alloc(alloc, sizeof(*t), (void **)&t) != MCE_OK) {
        return WM_ERROR_OUT_OF_MEMORY;
    }

    memset(t, 0, sizeof(*t));
    t->alloc = alloc;

    if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) {
        mc_free(alloc, t);
        return WM_ERROR_UNKNOWN;
    }

    HRESULT hr = CoCreateInstance(&WM_CLSID_ImmersiveShell, NULL, CLSCTX_LOCAL_SERVER,
                                  &IID_IServiceProvider, (void **)&t->sp);
    if (FAILED(hr) || !t->sp) {
        CoUninitialize();
        mc_free(alloc, t);
        return WM_ERROR_UNKNOWN;
    }

    hr = t->sp->lpVtbl->QueryService(t->sp,
                                     &WM_CLSID_VirtualDesktopManagerInternal,
                                     &WM_IID_IVirtualDesktopManagerInternal,
                                     (void **)&t->vdmi);
    if (FAILED(hr) || !t->vdmi) {
        t->sp->lpVtbl->Release(t->sp);
        CoUninitialize();
        mc_free(alloc, t);
        return WM_ERROR_UNKNOWN;
    }

    if (FAILED(CoCreateInstance(&WM_CLSID_VirtualDesktopManager, NULL, CLSCTX_INPROC_SERVER,
                                &WM_IID_IVirtualDesktopManager, (void **)&t->vdm))) {
        t->vdm = NULL;
    }

    if (FAILED(t->sp->lpVtbl->QueryService(t->sp, &WM_IID_IApplicationViewCollection,
                                           &WM_IID_IApplicationViewCollection, (void **)&t->avc))) {
        t->avc = NULL;
    }

    *out = t;
    return WM_ERROR_OK;
}

static WM_WindowChangeSink window_sink = NULL;
static void *window_sink_ctx = NULL;
static HWINEVENTHOOK window_event_hook = NULL;
static HWINEVENTHOOK foreground_event_hook = NULL;

static void win_destroy(WM_Target *t) {
    if (!t) {
        return;
    }

    if (foreground_event_hook) {
        UnhookWinEvent(foreground_event_hook);
        foreground_event_hook = NULL;
    }

    if (window_event_hook) {
        UnhookWinEvent(window_event_hook);
        window_event_hook = NULL;
        window_sink = NULL;
        window_sink_ctx = NULL;
    }

    WM_VDesktop **it;
    MC_VECTOR_EACH(t->items, it) {
        destroy_desktop(t, *it);
    }
    MC_VECTOR_FREE(t->items);
    t->items = NULL;

    if (t->avc) {
        t->avc->lpVtbl->Release(t->avc);
    }
    if (t->vdm) {
        t->vdm->lpVtbl->Release(t->vdm);
    }
    if (t->vdmi) {
        t->vdmi->lpVtbl->Release(t->vdmi);
    }
    if (t->sp) {
        t->sp->lpVtbl->Release(t->sp);
    }

    CoUninitialize();
    mc_free(t->alloc, t);
}

static WM_VDesktopSpan win_get_vdesk(WM_Target *t) {
    WM_VDesktopSpan span = { NULL, 0 };
    if (refresh(t) == WM_ERROR_OK) {
        span.desktops = MC_VECTOR_DATA(t->items);
        span.count = MC_VECTOR_SIZE(t->items);
    }

    return span;
}

static WM_Error win_vdesk_open(WM_Target *t, WM_VDesktop *d) {
    if (!d || !d->iface) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    if (FAILED(t->vdmi->lpVtbl->SwitchDesktop(t->vdmi, d->iface))) {
        return WM_ERROR_UNKNOWN;
    }

    return WM_ERROR_OK;
}

static WM_Error win_vdesk_create(WM_Target *t, MC_Str name, WM_VDesktop **out) {
    if (out == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    IVirtualDesktop *vd = NULL;
    if (FAILED(t->vdmi->lpVtbl->CreateDesktop(t->vdmi, &vd)) || !vd) {
        return WM_ERROR_UNKNOWN;
    }

    HSTRING hs = NULL;
    if (utf8_to_hstring(name, &hs) == WM_ERROR_OK) {
        t->vdmi->lpVtbl->SetDesktopName(t->vdmi, vd, hs);
        WindowsDeleteString(hs);
    }

    GUID id;
    memset(&id, 0, sizeof(id));
    vd->lpVtbl->GetID(vd, &id);
    vd->lpVtbl->Release(vd);

    if (refresh(t) != WM_ERROR_OK) {
        return WM_ERROR_UNKNOWN;
    }

    WM_VDesktop *d = find_desktop(t, &id);
    if (d == NULL) {
        return WM_ERROR_UNKNOWN;
    }

    *out = d;
    return WM_ERROR_OK;
}

static WM_VDesktop *win_vdesk_current(WM_Target *t) {
    if (MC_VECTOR_SIZE(t->items) == 0 && refresh(t) != WM_ERROR_OK) {
        return NULL;
    }

    IVirtualDesktop *cur = NULL;
    if (FAILED(t->vdmi->lpVtbl->GetCurrentDesktop(t->vdmi, &cur)) || !cur) {
        return NULL;
    }

    GUID id;
    memset(&id, 0, sizeof(id));
    cur->lpVtbl->GetID(cur, &id);
    cur->lpVtbl->Release(cur);

    WM_VDesktop **it;
    MC_VECTOR_EACH(t->items, it) {
        if (guid_eq(&(*it)->id, &id)) {
            return *it;
        }
    }

    return NULL;
}

static MC_Str win_vdesk_name(WM_Target *t, const WM_VDesktop *d) {
    (void)t;
    return mc_string_str(d ? d->name : NULL);
}

typedef struct WM_EnumVDeskCtx {
    IVirtualDesktopManager *vdm;
    GUID id;
    bool skip_cloaked;
    WM_WindowHandleSink sink;
    void *ctx;
} WM_EnumVDeskCtx;

static bool hwnd_is_cloaked(HWND hwnd) {
    DWORD cloaked = 0;
    return SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked != 0;
}

static BOOL CALLBACK enum_vdesk_windows(HWND hwnd, LPARAM lparam) {
    WM_EnumVDeskCtx *e = (WM_EnumVDeskCtx *)lparam;

    if (!IsWindowVisible(hwnd) || GetWindowTextLengthW(hwnd) == 0) {
        return TRUE;
    }

    if (e->skip_cloaked && hwnd_is_cloaked(hwnd)) {
        return TRUE;
    }

    GUID id;
    if (FAILED(e->vdm->lpVtbl->GetWindowDesktopId(e->vdm, hwnd, &id))) {
        return TRUE;
    }

    if (guid_eq(&id, &e->id)) {
        e->sink(e->ctx, (uint64_t)(uintptr_t)hwnd);
    }

    return TRUE;
}

static bool vdesk_is_current(WM_Target *t, const WM_VDesktop *vdesk) {
    IVirtualDesktop *cur = NULL;
    if (FAILED(t->vdmi->lpVtbl->GetCurrentDesktop(t->vdmi, &cur)) || !cur) {
        return false;
    }

    GUID id;
    memset(&id, 0, sizeof(id));
    cur->lpVtbl->GetID(cur, &id);
    cur->lpVtbl->Release(cur);

    return guid_eq(&id, &vdesk->id);
}

static WM_Error win_vdesk_windows(WM_Target *t, const WM_VDesktop *vdesk, WM_WindowHandleSink sink, void *ctx) {
    if (t == NULL || vdesk == NULL || sink == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    if (t->vdm == NULL) {
        return WM_ERROR_NOT_IMPLEMENTED;
    }

    WM_EnumVDeskCtx e = {
        .vdm = t->vdm,
        .id = vdesk->id,
        .skip_cloaked = vdesk_is_current(t, vdesk),
        .sink = sink,
        .ctx = ctx,
    };
    EnumWindows(enum_vdesk_windows, (LPARAM)&e);

    return WM_ERROR_OK;
}

static WM_Error win_vdesk_move_window(WM_Target *t, const WM_VDesktop *vdesk, uint64_t handle) {
    if (t == NULL || vdesk == NULL || vdesk->iface == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    if (t->avc == NULL) {
        return WM_ERROR_NOT_IMPLEMENTED;
    }

    IUnknown *view = NULL;
    if (FAILED(t->avc->lpVtbl->GetViewForHwnd(t->avc, (HWND)(uintptr_t)handle, &view)) || !view) {
        return WM_ERROR_UNKNOWN;
    }

    HRESULT hr = t->vdmi->lpVtbl->MoveViewToDesktop(t->vdmi, view, vdesk->iface);
    view->lpVtbl->Release(view);

    if (FAILED(hr)) {
        return WM_ERROR_UNKNOWN;
    }

    return WM_ERROR_OK;
}

static WM_Error win_vdesk_size(WM_Target *t, const WM_VDesktop *vdesk, MC_Size2U *out) {
    (void)t;
    (void)vdesk;

    RECT work;
    if (!SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0)) {
        return WM_ERROR_UNKNOWN;
    }

    *out = (MC_Size2U){
        .width = (unsigned)(work.right - work.left),
        .height = (unsigned)(work.bottom - work.top),
    };
    return WM_ERROR_OK;
}

static bool is_root_window(HWND hwnd) {
    if (GetAncestor(hwnd, GA_ROOT) != hwnd) {
        return false;
    }

    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    if (style & WS_CHILD) {
        return false;
    }

    LONG_PTR ex_style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (ex_style & WS_EX_TOOLWINDOW) {
        return false;
    }

    return true;
}

static void CALLBACK win_event_proc(HWINEVENTHOOK hook, DWORD event, HWND hwnd, LONG id_object, LONG id_child, DWORD thread, DWORD time) {
    (void)hook;
    (void)thread;
    (void)time;

    if (window_sink == NULL || hwnd == NULL) {
        return;
    }

    if (id_object != OBJID_WINDOW || id_child != CHILDID_SELF) {
        return;
    }

    if (event == EVENT_OBJECT_CREATE) {
        if (!is_root_window(hwnd)) {
            return;
        }
        window_sink(window_sink_ctx, (uint64_t)(uintptr_t)hwnd, WM_WINDOW_OPENED);
    } else if (event == EVENT_SYSTEM_FOREGROUND) {
        if (!is_root_window(hwnd)) {
            return;
        }
        window_sink(window_sink_ctx, (uint64_t)(uintptr_t)hwnd, WM_WINDOW_FOCUSED);
    } else if (event == EVENT_OBJECT_SHOW) {
        if (!is_root_window(hwnd)) {
            return;
        }
        window_sink(window_sink_ctx, (uint64_t)(uintptr_t)hwnd, WM_WINDOW_CREATED);
    } else if (event == EVENT_OBJECT_DESTROY) {
        window_sink(window_sink_ctx, (uint64_t)(uintptr_t)hwnd, WM_WINDOW_DESTROYED);
    }
}

static WM_Error win_on_window_changed(WM_Target *t, WM_WindowChangeSink sink, void *ctx) {
    (void)t;

    window_sink = sink;
    window_sink_ctx = ctx;

    if (window_event_hook == NULL) {
        window_event_hook = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_SHOW, NULL, win_event_proc, 0, 0, WINEVENT_OUTOFCONTEXT);
        if (window_event_hook == NULL) {
            return WM_ERROR_UNKNOWN;
        }
    }

    if (foreground_event_hook == NULL) {
        foreground_event_hook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, win_event_proc, 0, 0, WINEVENT_OUTOFCONTEXT);
        if (foreground_event_hook == NULL) {
            return WM_ERROR_UNKNOWN;
        }
    }

    return WM_ERROR_OK;
}

static const WM_TargetInterface windows_interface = {
    .capabilities = WM_TARGET_CAP_WINDOW_EVENTS,
    .init = win_init,
    .destroy = win_destroy,
    .get_vdesk = win_get_vdesk,
    .vdesk_open = win_vdesk_open,
    .vdesk_create = win_vdesk_create,
    .vdesk_current = win_vdesk_current,
    .vdesk_name = win_vdesk_name,
    .vdesk_windows = win_vdesk_windows,
    .vdesk_size = win_vdesk_size,
    .vdesk_move_window = win_vdesk_move_window,
    .on_window_changed = win_on_window_changed,
};

const WM_TargetInterface *const uniwm_windows_target = &windows_interface;
