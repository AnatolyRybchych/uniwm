#ifndef UNIWM_PROC_H
#define UNIWM_PROC_H

#include <stddef.h>
#include <stdint.h>

#include <uniwm/wm.h>

typedef struct WM_Child WM_Child;

WM_Error wm_proc_spawn_self(const char *const *args, size_t count, WM_Child **out);
WM_Error wm_proc_wait(WM_Child *child, int *exit_code);
void wm_proc_close(WM_Child *child);

uint64_t wm_proc_now_ms(void);
void wm_proc_sleep_ms(uint64_t ms);

#endif
