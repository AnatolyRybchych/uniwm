#ifndef WM_VDESKTOP_H
#define WM_VDESKTOP_H

#include <stddef.h>

#include <mc/data/vector.h>

typedef struct WM_VDesktop WM_VDesktop;
typedef struct WM_VDesktopSpan WM_VDesktopSpan;

struct WM_VDesktopSpan {
    WM_VDesktop **desktops;
    size_t count;
};

MC_DEFINE_VECTOR(WM_VDesktopList, WM_VDesktop *);

#endif
