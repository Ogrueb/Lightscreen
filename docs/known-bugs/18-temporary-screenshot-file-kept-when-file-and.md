# 18. Temporary `.screenshot.` file kept when file and clipboard are both on (lower severity)

- **Found:** found by the Stage 4 screenshot naming tests.
- **Where:** `Screenshot::save`, `Screenshot::unloadPixmap`.
- **What happens:** with the clipboard on, `save()` reloads the unloaded pixmap from the temp file and clears `mUnloaded`, so the file branch saves the pixmap again (a second encode) instead of renaming. The hidden `.screenshot.<n><ext>` file stays in the screenshot folder until the `Screenshot` is destroyed; a crash in between leaves it behind.
- **Fix:** keep the rename path when the pixmap was unloaded, reloading only for the clipboard.
- **Platform:** any. **Test:** `tst_screenshot::confirm_true_afterUnload_withClipboard`.
- **Triage:** fix now, with 16 (session D).
- **Status:** fixed in Stage 5 (2026-10-14). The clipboard gets a pixmap loaded from the temp file without clearing `mUnloaded`, so the file is renamed into place and no `.screenshot.` file is left.
