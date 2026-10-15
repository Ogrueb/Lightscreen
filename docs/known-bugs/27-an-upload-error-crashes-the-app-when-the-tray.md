# 27. An upload error crashes the app when the tray icon is off

- **Found:** found by the Stage 4 main window tests.
- **Where:** `lightscreenwindow.cpp:667-677` (`showUploaderError`) calls `notify(Screenshot::Failure)` unconditionally; `notify` (`:379-406`) calls `mTrayIcon->setIcon` without checking `mTrayIcon`, which is null when `options/tray` is off.
- **What happens:** with the tray icon disabled, any upload error (no pomf URL, unreadable file, network or host error, and every cancelled upload because of bug 22) dereferences the null tray icon and the app crashes. `cleanup` guards its own `notify` call with `options/tray && mTrayIcon`; `showUploaderError` does not.
- **Fix:** check `mTrayIcon` in `notify` (the window title part still applies), or guard the call as `cleanup` does.
- **Platform:** any. **Test:** confirmed under Xvfb by running `tst_mainwindow::uploadLastFlag` with `options/tray=false` (segfault in `QSystemTrayIcon::setIcon` from `notify`); the committed test turns the tray on so it can run, and can drop that line once this is fixed.
- **Triage:** fix now (session A); priority: crash.
- **Status:** fixed in Stage 5 (2026-10-14). `notify` sets the tray icon only when `mTrayIcon` exists; the window title (and the Windows taskbar overlay) still change. `tst_mainwindow::uploadErrorWithoutTray` runs an upload error with the tray off, and `uploadLastFlag` no longer turns the tray on.
