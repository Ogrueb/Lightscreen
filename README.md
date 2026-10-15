## Lightscreen (fork)
[![CI](https://github.com/Ogrueb/Lightscreen/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/Ogrueb/Lightscreen/actions/workflows/ci.yml)

A screenshot tool for Windows and Linux.

This is a fork of [Lightscreen](https://github.com/ckaiser/Lightscreen) by Christian Kaiser, which has not been updated in several years. The fork is being modernized in stages and will ship under a new name; until that name is chosen the code still says "Lightscreen". Please report problems with this fork here, not to the original project.

### What changed from upstream 2.5

- Builds on a current toolchain, with GitHub Actions CI on Windows and Linux on every push and pull request.
- A test suite (QtTest) covering settings, screenshot naming and saving, history, the uploader, the hotkey widget, the area selector, the main window and the OS helpers.
- Many known bugs fixed: see [docs/known-bugs.md](docs/known-bugs.md) and the files under `docs/known-bugs/`.
- The SingleApplication and UGlobalHotkey submodules are vendored under `tools/`, so a plain clone builds.
- Windows 10 is the minimum; code for older Windows versions and the XP build flags are gone.
- The Imgur uploader is removed; uploads go to a pomf-compatible host until the user-configurable uploader arrives.

Still to come (see [docs/modernization-plan.md](docs/modernization-plan.md)): Qt 6 with C++17, a CMake build, a user-configurable uploader in place of pomf, an update check against this fork's GitHub Releases, and the rename.

### Building

See [docs/build.md](docs/build.md) for Windows (MSVC) and Linux, and for running the tests. Every CI run on Windows also uploads a ready-to-run `lightscreen-win64` zip.

### License

GPL v2 (see [LICENSE](LICENSE)). `dialogs/areadialog.*` is based on KDE's KSnapshot and keeps its LGPL notice.
