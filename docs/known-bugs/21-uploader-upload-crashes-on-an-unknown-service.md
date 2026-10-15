# 21. `Uploader::upload` crashes on an unknown service name

- **Found:** found while writing the Stage 4 uploader tests.
- **Where:** `tools/uploader/uploader.cpp:93-128` (`Uploader::upload`); `ImageUploader::factory` (`tools/uploader/imageuploader.cpp:9-18`) returns `nullptr` for any name but `"imgur"` and `"pomf"`.
- **What happens:** `upload()` does not check the factory result: the `connect` calls warn, then `uploader->upload(fileName)` dereferences the null pointer and the app crashes. Nothing passes an unknown name today (`Uploader::serviceName` maps every index to `imgur` or `pomf`, and the history dialog's key from bug 14 is never written, so it gets the `"imgur"` default), but M12 and S03 rename the services and must keep `serviceName`, `factory` and the history dialog call in step.
- **Fix:** return early with `error` and `done(file, "", "")` when `factory` returns `nullptr`.
- **Platform:** any. **Test:** headless, `upload(file, "unknown")`; not in `tests/tst_uploader` today because it would crash the test. Confirmed from the code.
- **Triage:** deferred to M12 (factory falls back instead of returning `nullptr`); S03 keeps it. Both done-checks carry the test.
- **Status:** fixed in M12 (Stage 6, PR #35). `ImageUploader::factory` falls back to pomf for any name; `tst_uploader::uploadUnknownService` covers it.
