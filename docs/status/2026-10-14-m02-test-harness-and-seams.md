# 2026-10-14: M02 test harness and seams

- `lightscreen.pro` is split: the sources, Qt modules, compile flags, platform libraries and the two vendored `.pri` files are in `src.pri`; `lightscreen.pro` keeps the target, `main.cpp`, the icon, the manifest and the linker subsystem flags. `tests/tests.pro` (subdirs) and `tests/common.pri` build tests against the same `src.pri`. One placeholder test, `tests/tst_smoke`. How to build and run: `docs/build.md`, "Tests".
- Seams, as the plan specifies: `ScreenshotManager(QSettings *, historyPath, parent)` + `setInstance`; `Uploader::setInstance` + `setNetwork`; `Screenshot::extensionFor` + `uniqueName` (the collision loop from `save()` moved verbatim, quirks kept). Existing call sites are unchanged; no behaviour change intended.
- Verified in the cloud container (Ubuntu 24.04, Qt 5.15.13 from apt), on a fresh clone of the branch: the app builds with the same warnings as `master`, and it starts and stops with `--quit` under Xvfb (the M01 smoke script); the tests build and `tst_smoke` passes under `xvfb-run -a make check` and with `QT_QPA_PLATFORM=offscreen`.
- CI (M01's workflow) now builds the tests and runs `make check` in both jobs: Linux under `xvfb-run -a`, Windows with `QT_QPA_PLATFORM=offscreen`. Windows is verified only through CI; nobody ran it on a Windows machine.
- Stage 4 can start once this is merged.
