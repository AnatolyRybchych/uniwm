#ifndef UNIWM_KEY_H
#define UNIWM_KEY_H

#include <stddef.h>

#include <mc/wm/key.h>

#define WM_KEY_COMBO_MAX 4

typedef struct WM_KeyCombo {
    MC_Key keys[WM_KEY_COMBO_MAX];
    size_t count;
} WM_KeyCombo;

#endif
