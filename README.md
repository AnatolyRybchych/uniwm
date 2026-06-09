# Disabling the desktop-switch animation

uniwm switches virtual desktops with a single COM call (near-instant) — the
lingering slide you still see is **Windows' own animation**, a system-wide setting
(not uniwm). Toggle it from `cmd`:

```cmd
:: disable
reg.exe add "HKCU\Control Panel\Desktop\WindowMetrics" /v MinAnimate /t REG_SZ /d 0 /f

:: re-enable
reg.exe add "HKCU\Control Panel\Desktop\WindowMetrics" /v MinAnimate /t REG_SZ /d 1 /f

:: apply without signing out (restart Explorer)
taskkill.exe /f /im explorer.exe & start explorer.exe
```

If the slide remains, also turn off **Settings → Accessibility → Visual effects →
Animation effects** (this is system-wide too).
