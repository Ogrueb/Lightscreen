# 2026-10-14: Stage 5, bug 27 fixed (upload error with the tray icon off)

- `LightscreenWindow::notify` no longer dereferences a null `mTrayIcon`: with `options/tray` off, an upload error (or a cancelled upload, bug 22) used to crash the app. The title and taskbar overlay are unchanged.
- New test `tst_mainwindow::uploadErrorWithoutTray` (segfaults before the fix); `uploadLastFlag` now runs with the tray off. All suites pass under Xvfb in the cloud container.
- Noticed, not changed: the "Failed!" title from `notify` is overwritten at once, because `Uploader::done` follows every error and `showUploaderMessage` calls `updateStatus`. So the failure title never shows after an upload error; the tray icon's red state (when the tray is on) does.
