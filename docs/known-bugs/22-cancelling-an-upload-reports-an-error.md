# 22. Cancelling an upload reports an error

- **Found:** found while writing the Stage 4 uploader tests.
- **Where:** `tools/uploader/pomfuploader.cpp:84` (cancel aborts the reply) and `:94-99` (the `finished` handler).
- **What happens:** `Uploader::cancel` makes each pomf uploader abort its reply. An aborted reply finishes with `OperationCanceledError` and an empty body, which the handler reports as `NetworkError` "Error reaching uploader" instead of `CancelError`. `Uploader` only suppresses `CancelError`, so every cancelled upload emits `error("Error reaching uploader")` then `done(file, "", "")`, and the main window shows an upload error after the user cancelled.
- **Fix:** in the pomf (or S03 custom) uploader, map `OperationCanceledError` to `CancelError`.
- **Platform:** any. **Test:** recorded by `tst_Uploader::cancel` (with a stub reply whose `abort()` behaves like Qt's HTTP reply); the fix flips that assertion. Not reproduced in the running app.
- **Triage:** deferred to S03 (the uploader rewrite); its done-check flips `tst_Uploader::cancel`.
- **Status:** fixed in S03 (Stage 6). `CustomUploader` reports an aborted reply as `CancelError`; `tst_Uploader::cancel` now expects `done` with no `error`, and the options dialog's "Test upload" shows "Cancelled".
