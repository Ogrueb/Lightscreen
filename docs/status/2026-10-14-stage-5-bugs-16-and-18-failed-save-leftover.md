# 2026-10-14: Stage 5, bugs 16 and 18 (failed save, leftover temp file)

- `Screenshot::save` emits `finished` once on `Failure` and returns before `saveHistory`, `optimize` and `upload` (bug 16). With file and clipboard both on, the clipboard gets the pixmap reloaded from the temporary file and the file branch renames it as before, so no `.screenshot.` file lingers (bug 18).
- `tst_screenshot::save_failure`, `save_neither` and `confirm_true_afterUnload_withClipboard` now assert the fixed behaviour. All suites pass on Linux.
- Left open, recorded under bug 16: an upload-only screenshot still reports `Failure` and emits `finished` twice; its early `finished` is what ends the screenshot when an upload fails, so it belongs with the uploader work.
