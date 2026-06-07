#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <windows.h>
#include <objbase.h>
#include <servprov.h>
#include <objectarray.h>
#include <winstring.h>

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
    HRESULT (STDMETHODCALLTYPE *GetAllCurrentDesktops)(IVirtualDesktopManagerInternal *this, IObjectArray **desktops);
    HRESULT (STDMETHODCALLTYPE *GetAdjacentDesktop)(IVirtualDesktopManagerInternal *this, IVirtualDesktop *from, int direction, IVirtualDesktop **desktop);
    HRESULT (STDMETHODCALLTYPE *SwitchDesktop)(IVirtualDesktopManagerInternal *this, IVirtualDesktop *desktop);
    HRESULT (STDMETHODCALLTYPE *CreateDesktop)(IVirtualDesktopManagerInternal *this, IVirtualDesktop **desktop);
    HRESULT (STDMETHODCALLTYPE *MoveDesktop)(IVirtualDesktopManagerInternal *this, IVirtualDesktop *desktop, UINT index);
    HRESULT (STDMETHODCALLTYPE *RemoveDesktop)(IVirtualDesktopManagerInternal *this, IVirtualDesktop *remove, IVirtualDesktop *fallback);
    HRESULT (STDMETHODCALLTYPE *FindDesktop)(IVirtualDesktopManagerInternal *this, const GUID *id, IVirtualDesktop **desktop);
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

    *out = t;
    return WM_ERROR_OK;
}

static void win_destroy(WM_Target *t) {
    if (!t) {
        return;
    }

    WM_VDesktop **it;
    MC_VECTOR_EACH(t->items, it) {
        destroy_desktop(t, *it);
    }
    MC_VECTOR_FREE(t->items);
    t->items = NULL;

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

static WM_Error win_vdesk_new(WM_Target *t, WM_VDesktop **out) {
    if (t == NULL || out == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    IVirtualDesktop *vd = NULL;
    if (FAILED(t->vdmi->lpVtbl->CreateDesktop(t->vdmi, &vd)) || !vd) {
        return WM_ERROR_UNKNOWN;
    }

    WM_VDesktop *d = wrap_desktop(t, vd, MC_VECTOR_SIZE(t->items));
    if (!d) {
        vd->lpVtbl->Release(vd);
        return WM_ERROR_OUT_OF_MEMORY;
    }

    WM_Error e = push_desktop(t, d);
    if (e != WM_ERROR_OK) {
        destroy_desktop(t, d);
        return e;
    }

    *out = d;
    return WM_ERROR_OK;
}

static WM_Error win_vdesk_delete(WM_Target *t, WM_VDesktop *d) {
    if (!d || !d->iface) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    IVirtualDesktop *fallback = NULL;
    WM_VDesktop **it;
    MC_VECTOR_EACH(t->items, it) {
        if (*it != d && (*it)->iface) {
            fallback = (*it)->iface;
            break;
        }
    }

    if (FAILED(t->vdmi->lpVtbl->RemoveDesktop(t->vdmi, d->iface, fallback))) {
        return WM_ERROR_UNKNOWN;
    }

    return WM_ERROR_OK;
}

static void pump_wait(DWORD ms) {
    DWORD start = GetTickCount();
    while (GetTickCount() - start < ms) {
        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        Sleep(5);
    }
}

static void claim_foreground(HWND decoy) {
    HWND foreground = GetForegroundWindow();
    DWORD foreground_thread = GetWindowThreadProcessId(foreground, NULL);
    DWORD this_thread = GetCurrentThreadId();

    AttachThreadInput(this_thread, foreground_thread, TRUE);
    SetForegroundWindow(decoy);
    AttachThreadInput(this_thread, foreground_thread, FALSE);
}

static WM_Error win_vdesk_open(WM_Target *t, WM_VDesktop *d) {
    if (!d || !d->iface) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    HWND decoy = CreateWindowExA(0, "STATIC", "", WS_POPUP, -32000, -32000, 1, 1, NULL, NULL, GetModuleHandleA(NULL), NULL);
    if (decoy != NULL) {
        ShowWindow(decoy, SW_SHOWNA);
        claim_foreground(decoy);
        pump_wait(150);
    }

    HRESULT hr = t->vdmi->lpVtbl->SwitchDesktop(t->vdmi, d->iface);

    if (decoy != NULL) {
        DestroyWindow(decoy);
    }

    if (FAILED(hr)) {
        return WM_ERROR_UNKNOWN;
    }

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

static const WM_TargetInterface windows_interface = {
    .init = win_init,
    .destroy = win_destroy,
    .vdesk_new = win_vdesk_new,
    .vdesk_delete = win_vdesk_delete,
    .get_vdesk = win_get_vdesk,
    .vdesk_open = win_vdesk_open,
    .vdesk_current = win_vdesk_current,
    .vdesk_name = win_vdesk_name,
};

const WM_TargetInterface *const uniwm_windows_target = &windows_interface;
