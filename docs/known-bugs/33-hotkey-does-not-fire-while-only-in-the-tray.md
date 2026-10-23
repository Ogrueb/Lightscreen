# 33. Global hotkeys do not fire while Lightscreen Rd is only in the tray

- **Found:** by the owner in the Windows QA pass (A2, `2026-10-22-fresh-install.md` step 3), 2026-10-23, on Windows 11 with the CI build from `master` at `06db33a` (run #179).
- **What happens:** with Windows' "Use the Print screen key to open screen capture" turned off, Print Screen captures with Lightscreen Rd only while its window or menu is open. With the app only in the tray, nothing happens. (With that Windows setting on, the Snipping Tool takes the key: a known issue, see below.)
- **Where:** `tools/UGlobalHotkey/uglobalhotkeys.cpp` (Windows: `RegisterHotKey` against the `UGlobalHotkeys` widget's own handle, `winId()`, a native child of `LightscreenWindow`; `WM_HOTKEY` arrives through `nativeEvent`), `LightscreenWindow::connectHotkeys`.
- **Narrowed down (owner, 2026-10-23):** every hotkey fails from the tray, not only Print Screen, and upstream 2.5 works from the tray on the same machine. So it is a regression from the Qt 6 port, not Windows' handling of Print Screen.
- **Likely cause (inferred; there is no Windows machine in the cloud to confirm):** the hotkeys are registered to the `UGlobalHotkeys` widget's own handle, a native child of the main window, and arrive through `QWidget::nativeEvent`. On Qt 6, `WM_HOTKEY` for that child evidently stops reaching the widget once its top-level window is hidden. The exact Qt mechanism is not confirmed.
- **Fix:** hotkeys are registered to the thread, `RegisterHotKey(nullptr, ...)`, so `WM_HOTKEY` arrives as a thread message, and they are caught by a `QAbstractNativeEventFilter` on `qApp`, as the Linux side of the same class already does. No window, visible or not, is involved. Ids this object did not register are ignored. Recorded in `tools/UGlobalHotkey/VENDORED.md`.
- **Test:** `tests/tst_globalhotkeys` (Windows; skipped elsewhere) posts the `WM_HOTKEY` thread message Windows sends and checks `activated()`: once for a registered hotkey, once with the parent window hidden, and never for an unknown or unregistered id. It checks the new delivery path; it cannot press a real key, so the owner's re-check is the real verification.
- **Trade-off:** while a native Windows modal loop runs (for example the "Save as" file dialog), thread messages are not delivered to Qt, so a hotkey pressed then is lost. Hotkeys registered to a window would be dispatched there. Qt's own menus and dialogs are not affected.
- **Platform:** Windows 11 (seen); Windows 10 unknown.
- **Status:** fix landed (2026-10-23), awaiting the owner's re-check: `docs/windows-qa/2026-10-22-fresh-install.md`, and `2026-10-20-qt6-flip.md` step 3 (hotkeys with the app in the background).

## Known issue, not a bug in the app

With Windows 11's "Use the Print screen key to open screen capture" on (the default), Windows' Snipping Tool takes Print Screen before any application. The user either turns that setting off (Settings > Accessibility > Keyboard) or gives Lightscreen Rd another hotkey. Listed in `CHANGELOG.md` and the release notes.
