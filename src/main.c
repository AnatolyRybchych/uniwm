#include <stdio.h>

#include <mc/data/alloc.h>
#include <mc/data/str.h>

#include <uniwm/wm.h>
#include <uniwm/target.h>
#include <uniwm-windows/target.h>

static void print_desktops(WM *wm) {
    WM_VDesktopSpan span = wm_vdesktops(wm);
    WM_VDesktop *current = wm_vdesktop_current(wm);

    if (span.count == 0) {
        puts("  (none detected)");
        return;
    }

    for (size_t i = 0; i < span.count; i++) {
        MC_Str name = wm_vdesktop_name(wm, span.desktops[i]);
        printf("  %c [%zu] %.*s\n",
               span.desktops[i] == current ? '*' : ' ', i + 1,
               (int)MC_STR_LEN(name), name.beg);
    }
}

int main(void) {
    WM wm;
    if (wm_init(&wm, uniwm_windows_target, &mc_alloc_malloc) != WM_ERROR_OK) {
        fprintf(stderr, "uniwm: failed to initialise target\n");
        return 1;
    }

    puts("Existing virtual desktops:");
    print_desktops(&wm);

    WM_VDesktop *created = NULL;
    if (wm_vdesktop_create(&wm, &created) != WM_ERROR_OK) {
        fprintf(stderr, "uniwm: failed to create a virtual desktop\n");
        wm_fini(&wm);
        return 1;
    }
    puts("\nCreated a new virtual desktop; switching to it...");
    if (wm_vdesktop_switch(&wm, created) != WM_ERROR_OK) {
        fprintf(stderr, "uniwm: failed to switch to the new desktop\n");
    }

    puts("\nVirtual desktops now (* = current):");
    print_desktops(&wm);

    wm_fini(&wm);

    return 0;
}
