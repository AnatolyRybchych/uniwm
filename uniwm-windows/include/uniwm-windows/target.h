#ifndef UNIWM_WINDOWS_TARGET_H
#define UNIWM_WINDOWS_TARGET_H

#include <uniwm/target.h>

#ifdef UNIWM_WINDOWS_BUILD
#define UNIWM_WINDOWS_API __declspec(dllexport)
#else
#define UNIWM_WINDOWS_API __declspec(dllimport)
#endif

extern UNIWM_WINDOWS_API const WM_TargetInterface *const uniwm_windows_target;

#endif
