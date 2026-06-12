#include "proc.h"

#include <stdlib.h>
#include <stdbool.h>

#include <windows.h>

#define COMMAND_LINE_CAP 32768
#define ARG_WIDE_CAP 4096

struct WM_Child {
    HANDLE process;
    HANDLE thread;
};

static HANDLE ensure_job(void) {
    static HANDLE job = NULL;
    static bool tried = false;

    if (tried) {
        return job;
    }
    tried = true;

    HANDLE created = CreateJobObjectW(NULL, NULL);
    if (created == NULL) {
        return NULL;
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = {0};
    info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(created, JobObjectExtendedLimitInformation, &info, sizeof(info))) {
        CloseHandle(created);
        return NULL;
    }

    job = created;
    return job;
}

static void append_quoted(wchar_t *buf, size_t cap, size_t *len, const wchar_t *s) {
    size_t i = *len;

    if (i + 1 < cap) {
        buf[i++] = L'"';
    }

    for (const wchar_t *p = s; *p != 0 && i + 2 < cap; p++) {
        if (*p == L'"') {
            buf[i++] = L'\\';
        }
        buf[i++] = *p;
    }

    if (i + 1 < cap) {
        buf[i++] = L'"';
    }

    *len = i;
}

WM_Error wm_proc_spawn_self(const char *const *args, size_t count, WM_Child **out) {
    if (out == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    wchar_t self[MAX_PATH];
    DWORD n = GetModuleFileNameW(NULL, self, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        return WM_ERROR_UNKNOWN;
    }

    wchar_t cmdline[COMMAND_LINE_CAP];
    size_t len = 0;
    append_quoted(cmdline, COMMAND_LINE_CAP, &len, self);

    for (size_t i = 0; i < count; i++) {
        wchar_t warg[ARG_WIDE_CAP];
        if (MultiByteToWideChar(CP_UTF8, 0, args[i], -1, warg, ARG_WIDE_CAP) == 0) {
            return WM_ERROR_UNKNOWN;
        }

        if (len + 1 < COMMAND_LINE_CAP) {
            cmdline[len++] = L' ';
        }
        append_quoted(cmdline, COMMAND_LINE_CAP, &len, warg);
    }
    cmdline[len < COMMAND_LINE_CAP ? len : COMMAND_LINE_CAP - 1] = 0;

    HANDLE job = ensure_job();

    STARTUPINFOW si = { .cb = sizeof(si) };
    PROCESS_INFORMATION pi = {0};
    if (!CreateProcessW(NULL, cmdline, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
        return WM_ERROR_UNKNOWN;
    }

    if (job != NULL) {
        AssignProcessToJobObject(job, pi.hProcess);
    }
    ResumeThread(pi.hThread);

    WM_Child *child = malloc(sizeof(*child));
    if (child == NULL) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return WM_ERROR_OUT_OF_MEMORY;
    }

    child->process = pi.hProcess;
    child->thread = pi.hThread;

    *out = child;
    return WM_ERROR_OK;
}

WM_Error wm_proc_wait(WM_Child *child, int *exit_code) {
    if (child == NULL || exit_code == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    if (WaitForSingleObject(child->process, INFINITE) != WAIT_OBJECT_0) {
        return WM_ERROR_UNKNOWN;
    }

    DWORD code = 0;
    GetExitCodeProcess(child->process, &code);
    *exit_code = (int)code;

    return WM_ERROR_OK;
}

void wm_proc_close(WM_Child *child) {
    if (child == NULL) {
        return;
    }

    CloseHandle(child->thread);
    CloseHandle(child->process);
    free(child);
}

uint64_t wm_proc_now_ms(void) {
    return GetTickCount64();
}

void wm_proc_sleep_ms(uint64_t ms) {
    Sleep((DWORD)ms);
}
