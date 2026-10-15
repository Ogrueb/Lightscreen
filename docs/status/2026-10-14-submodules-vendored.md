# 2026-10-14: Submodules vendored

- SingleApplication and UGlobalHotkey are now plain files in the repo instead of submodules (see `docs/decisions.md`). A plain `git clone` is enough to build; the `git submodule update --init` step is gone from `docs/build.md`.
- Linux build check, done in the cloud container (Ubuntu 24.04, Qt 5.15.13 from apt): `qmake` and `make` succeed with the vendored sources, and the binary starts and keeps running under Xvfb. Two findings for whoever owns the Linux build:
  - `qtbase5-private-dev` is needed in addition to the packages listed in `docs/build.md`, because UGlobalHotkey includes `qpa/qplatformnativeinterface.h`.
  - With `QT_QPA_PLATFORM=offscreen` (no X server) the app segfaults at startup; it needs a real or virtual X display.
- Windows build verified by the owner on a fresh clone (VS 2022, Qt 5.15.2): builds, launches, same 14 warnings as Stage 1. Existing checkouts should run `git submodule deinit --all` (or reclone) after pulling this change.
