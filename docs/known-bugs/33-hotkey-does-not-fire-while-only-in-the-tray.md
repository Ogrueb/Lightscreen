# 33. The Print Screen hotkey does not fire while Lightscreen Rd is only in the tray

- **Found:** by the owner in the Windows QA pass (A2, `2026-10-22-fresh-install.md` step 3), 2026-10-23, on Windows 11 with the CI build of PR #58.
- **What happens:** with Windows' "Use the Print screen key to open screen capture" turned off, Print Screen captures with Lightscreen Rd only while its window or menu is open. With the app only in the tray, nothing happens. (With that Windows setting on, the Snipping Tool takes the key: a known issue, see below.)
- **Where:** `tools/UGlobalHotkey/uglobalhotkeys.cpp` (Windows: `RegisterHotKey` against the `UGlobalHotkeys` widget's own handle, `winId()`, a native child of `LightscreenWindow`; `WM_HOTKEY` arrives through `nativeEvent`), `LightscreenWindow::connectHotkeys`.
- **Possible causes, not yet told apart:**
  1. **Print Screen only.** Windows 11 treats `VK_SNAPSHOT` specially. `RegisterHotKey` for it may only see the key when a Lightscreen window has focus; tools that catch it reliably use a low-level keyboard hook (`WH_KEYBOARD_LL`).
  2. **Every hotkey, in the background.** Something in the Qt 6 port stops `WM_HOTKEY` reaching `UGlobalHotkeys::nativeEvent` while no Lightscreen window is visible. That would be a regression from 2.5, where hotkeys from the tray were the main way to use the app.
- **To tell them apart (owner):**
  - with the app only in the tray, does another hotkey such as Ctrl+Shift+F5 capture?
  - does upstream 2.5 capture with Print Screen from the tray on the same machine?
- **Platform:** Windows 11 (seen); Windows 10 unknown.
- **Status:** open, under investigation.

## Known issue, not a bug in the app

With Windows 11's "Use the Print screen key to open screen capture" on (the default), Windows' Snipping Tool takes Print Screen before any application. The user either turns that setting off (Settings > Accessibility > Keyboard) or gives Lightscreen Rd another hotkey. Listed in `CHANGELOG.md` and the release notes.
