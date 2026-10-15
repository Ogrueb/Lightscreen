# 16. A failed save emits `finished` twice and still writes a history row

- **Found:** found by the Stage 4 screenshot naming tests.
- **Where:** `tools/screenshot.cpp`, `Screenshot::save`.
- **What happens:** `if (!mOptions.result) emit finished();` fires for `Failure`, and the branch below then emits `finished` again (after `saveHistory` when file saving is on). `ScreenshotManager::finished` runs twice for one screenshot (`removeOne`, `activeCountChange` and `deleteLater` twice), and the history gets a row for a file that was never written. Same double `finished` when neither file nor clipboard is on.
- **Fix:** return after the failure `finished`, or emit it once at the end; skip `saveHistory` on failure.
- **Platform:** any. **Test:** `tst_screenshot::save_failure`, `tst_screenshot::save_neither` (confirmed).
- **Triage:** fix now (Stage 5 session D, with 18), after the bug 10 PR merges.
- **Status:** fixed in Stage 5 (2026-10-14). On `Failure`, `save()` emits `finished` once and returns: no history row, no optimize or upload hand-off. Left open: upload-only (file off and no clipboard image) still reports `Failure` and emits `finished` early, then again when the upload succeeds. That early `finished` currently ends the screenshot when an upload fails (the uploader emits no `done` on error), so it is left for the uploader work rather than removed here.
