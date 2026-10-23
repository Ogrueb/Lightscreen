# 39. WEBP screenshots wrote no file with the Windows package

- **Found:** by the owner in the Windows QA pass (A12, `2026-10-15-webp-format.md` step 1), 2026-10-23, Windows 11: with WEBP chosen, a screenshot writes no file. The setting itself is kept (step 2 passed).
- **Cause:** Qt's WEBP plugin (`imageformats\qwebp.dll`) is in the `qtimageformats` add-on module, which the CI and release workflows did not install, so `windeployqt` had nothing to ship. `QImage::save` then fails for `.webp`. The check had already allowed for this ("if no file appears, note that").
- **Fix:** both Windows workflows install Qt with `modules: qtimageformats`, and fail unless `windeployqt` deployed `qwebp.dll`. `docs/build.md` adds `-m qtimageformats` to the local `aqt` command.
- **Test:** on Windows, `tst_Screenshot::save_formats` now requires a writer for every format, WEBP included (it used to accept a failed WEBP save when the plugin was missing; Linux keeps that).
- **Platform:** Windows package. **Status:** fixed, awaiting the owner's re-check (`2026-10-15-webp-format.md`).
