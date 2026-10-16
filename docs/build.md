# Building Lightscreen

Status as of Stage 1 (2026-10-14): the Windows x64 release build compiles, links and launches on a current toolchain **with no source or build-file changes**. Since M01, both the Windows and the Linux build run in GitHub Actions on every push (see "Continuous integration" below). Upstream had no tests; the test harness added in M02 is described under [Tests](#tests), and CI runs it on both platforms.

## Windows (verified)

### Toolchain

| Component | Version | Installed with |
|---|---|---|
| Visual Studio 2022 Build Tools, "Desktop development with C++" workload | 17.14 (MSVC 19.44.35229, x64) | `winget` |
| Qt | 5.15.2, `msvc2019_64` kit (binary-compatible with MSVC 2022) | `aqtinstall` 3.3.0 |
| Python (only to run aqtinstall) | 3.12 | already present |

Qt 5.15.2 is the last Qt 5 release with official open-source Windows binaries, and the code targets Qt 5, so it is the version that needs the fewest changes. All required Qt modules (core, gui, widgets, network, sql, concurrent, winextras; `winextras` is linked on Qt 5 only, for the bitmap and icon conversions, since M07) are in the base `msvc2019_64` package; no extra `-m` modules are needed.

### One-time setup

```powershell
winget install --id Microsoft.VisualStudio.2022.BuildTools --exact --source winget --override "--wait --passive --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
python -m pip install --user aqtinstall
python -m aqt install-qt windows desktop 5.15.2 win64_msvc2019_64 --outputdir C:\Qt
```

The Build Tools install needs an admin prompt (about 6 GB). Qt goes into `C:\Qt` (about 1.5 GB, no admin). Run the `aqt` command from outside the checkout: it writes `aqtinstall.log` to the current directory.

### Build

From a plain `cmd` prompt in the checkout:

```bat
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
set PATH=C:\Qt\5.15.2\msvc2019_64\bin;%PATH%
mkdir build && cd build
qmake ..\lightscreen.pro CONFIG+=release
nmake
```

Output: `build\release\lightscreen.exe`. `build/` is git-ignored.

### Run / package

The exe needs the Qt DLLs next to it:

```bat
mkdir deploy && copy release\lightscreen.exe deploy\
windeployqt --release --no-translations deploy\lightscreen.exe
deploy\lightscreen.exe
```

Smoke test done in Stage 1: the deployed exe starts and keeps running (it sits in the tray). No functional QA yet.

## Linux (verified in CI)

Ubuntu 24.04 with Qt 5.15.13 from apt. This is what the CI `linux` job runs:

```sh
sudo apt-get install qtbase5-dev libqt5x11extras5-dev \
  libqt5sql5-sqlite libx11-dev libxcb-keysyms1-dev xvfb
mkdir build && cd build
qmake ../lightscreen.pro
make -j"$(nproc)"
```

`libqt5x11extras5-dev` (`QX11Info`) is needed on Qt 5 only: on Qt 6 the X display and xcb connection come from `QNativeInterface::QX11Application` in QtGui (M08). No private Qt headers are needed any more (until M08, UGlobalHotkey included `qpa/qplatformnativeinterface.h`). `xdotool` is only for the hotkey smoke in CI. The app needs an X display (real or `xvfb-run`); with `QT_QPA_PLATFORM=offscreen` it segfaults at startup.

## Continuous integration

`.github/workflows/ci.yml` runs on every pull request and every push to `master`, with two jobs:

- **linux** (`ubuntu-latest`): installs the packages above, builds, then runs the *Linux smoke*. It writes a `config.ini` next to the binary (portable mode, so no user settings are touched) with `file/format`, `file/enabled` and `file/target` set so the first-run options dialog is skipped, `options/tray=false` so no system tray is needed, and `options/disableUpdater=true` so the run makes no network calls. Under `xvfb-run -a` it starts `./lightscreen`, waits two seconds, runs `./lightscreen --quit` (SingleApplication forwards it to the running instance), and fails unless the first process exits with status 0 within ten seconds. It then runs `./lightscreen --quit` again with no instance running, which must exit with status 0 within ten seconds (bug 29). A second step, the *hotkey smoke*, starts the app with the whole-screen action on Print Screen (`actions/screen/enabled=true`, `actions/screen/hotkey=Print`), sends the key with `xdotool key Print`, and fails unless a PNG appears in the target folder within ten seconds; this covers the UGlobalHotkeys X key grab end to end.
- **windows** (`windows-latest`): Qt 5.15.2 `win64_msvc2019_64` from `jurplel/install-qt-action` (cached), MSVC x64 from `ilammy/msvc-dev-cmd`, `qmake CONFIG+=release` and `nmake`, then `windeployqt --release --no-translations`. The deploy folder is uploaded as the `lightscreen-win64` artifact on the run's page; unzip it and run `lightscreen.exe`.
- Both jobs also build `tests/tests.pro` and run `make check` / `nmake check` (see [Tests](#tests)): Linux under `xvfb-run -a`, Windows with `QT_QPA_PLATFORM=offscreen`. A failing test fails the job.

To run the smoke locally after a Linux build, copy the script from the "Smoke test" step of the workflow into `build/smoke.sh` and run `xvfb-run -a bash smoke.sh` from `build/` (the same for the "Hotkey smoke" step, which needs `xdotool`). If a previous run was killed, a stray `lightscreen` process will make the next run forward its arguments and exit; kill it first.

## What broke and how it was fixed

Nothing needed fixing to build. Things noticed along the way, left unchanged on purpose:

- **Windows XP targeting flags and the MinGW branch** (`_ATL_XP_TARGETING`, `/D _USING_V110_SDK71`, `/SUBSYSTEM:...,5.01`, which caused the linker warning `LNK4010`, and an unmaintained MinGW branch) were removed in M03, when the minimum became Windows 10. Only MSVC is supported on Windows.
- **README badges** pointed at the original author's AppVeyor and Coverity projects. Replaced by the fork's CI badge in M01; M14 rewrote the README as a fork notice that points here for building.
- **Dead files.** `dialogs/aboutdialog.ui` (not in `FORMS`, never used) and the `OTHER_FILES += TODO.txt` line in `lightscreen.pro` (the file never existed) were removed in M14.
- **Compiler warnings worth a look** (14 in total, the rest are `size_t` to `int` narrowing in UGlobalHotkey and `lightscreenwindow.cpp:113`):
  - `tools/windowpicker.cpp:168` (C4312): `GetClassLong(..., GCL_HICON)` returns a 32-bit value, which truncates the icon handle on 64-bit builds. `GetClassLongPtr` / `GCLP_HICON` is the 64-bit-safe call. Likely a real bug in the window picker's icon on x64.
  - `tools/screenshot.cpp:404` (C4715): `Screenshot::extension()` has no return for formats outside its `switch` (WEBP was removed from the UI but is still in the enum). Undefined behavior if it is ever hit.
- **`QT_DEPRECATED_WARNINGS`** is defined, but against Qt 5.15.2 the build emits no C4996 deprecation warnings. Qt 6 porting issues (`winextras`, `x11extras`, `QRegExp`, `QDesktopWidget`) will only show up when building against Qt 6.

## Tests

The tests live under `tests/`, one directory per test (`tests/tst_<name>/`), all listed in `tests/tests.pro` (`TEMPLATE = subdirs`). Each test's `.pro` includes `tests/common.pri`, which includes `src.pri`: the tests compile the application's own sources (everything but `main.cpp`) rather than linking a library. `lightscreen.pro` includes the same `src.pri`, so a file added to the app goes into `src.pri`, not `lightscreen.pro`.

`tst_smoke` checks that the harness builds and that the seams below can be installed. The Stage 4 safety-net tests sit next to it, one directory per module: `tst_settings`, `tst_screenshot`, `tst_history`, `tst_uploader`, `tst_hotkeywidget`, `tst_areadialog`, `tst_mainwindow` and `tst_os` (what each records: `docs/status.md`). Stage 6 adds `tst_updater` (M13: the GitHub Releases update check against a stub network). `tst_mainwindow` and the window tests in `tst_settings` skip on Windows and offscreen.

### Running the tests

Linux, from the checkout (`build-tests/` is git-ignored):

```sh
mkdir build-tests && cd build-tests
qmake ../tests/tests.pro
make -j"$(nproc)"
xvfb-run -a make check
```

Windows, from a `cmd` prompt set up as in the Windows build section:

```bat
mkdir build-tests && cd build-tests
qmake ..\tests\tests.pro CONFIG+=release
nmake
set QT_QPA_PLATFORM=offscreen
nmake check
```

`make check` builds nothing new; it runs each test and fails on the first failing one. A single test can be run directly, e.g. `./tst_smoke/tst_smoke` (Linux) or `tst_smoke\release\tst_smoke.exe` (Windows), with the usual QtTest options (`-v2`, a test function name).

Tests can run with `QT_QPA_PLATFORM=offscreen` except those that construct `LightscreenWindow`: it creates `UGlobalHotkeys`, which needs a real X display on Linux (see `docs/status.md`), so those need `xvfb-run`.

### Writing a test

Each test starts with `QStandardPaths::setTestModeEnabled(true)` and `QSettings::setDefaultFormat(QSettings::IniFormat)` so it never touches the developer's settings or data, works in a `QTemporaryDir`, and installs its own singletons through the seams:

- `ScreenshotManager(QSettings *settings, const QString &historyPath, QObject *parent)` builds a manager on the given settings (not owned) and history directory (with a trailing separator), then `ScreenshotManager::setInstance(&manager)` makes it the one `ScreenshotManager::instance()` returns.
- `Uploader::setInstance(&uploader)` does the same for the uploader, and `uploader.setNetwork(&nam)` makes every upload and network call use `nam` (not owned), typically a `QNetworkAccessManager` subclass whose `createRequest` returns a canned reply.
- `Screenshot::extensionFor(Format)` and `Screenshot::uniqueName(nameWithoutExtension, extension)` expose the extension lookup and the " (n)" collision naming used by `Screenshot::save()`.

`setInstance` does not delete the previous instance; reset it to `nullptr` at the end of the test so a destroyed object is not left installed.

## Known flaky tests

None so far.

## Third-party libraries

SingleApplication and UGlobalHotkey used to be git submodules pointing at the original author's repos. They are now vendored (copied unchanged) under `tools/SingleApplication` and `tools/UGlobalHotkey`, so a plain clone builds with no `git submodule` step. Each directory has a `VENDORED.md` with its origin and pinned commit. The reasoning is in `docs/decisions.md`.

## Update check

The update check (`updater/updater.cpp`) asks `https://api.github.com/repos/<UPDATE_REPO>/releases/latest` and compares the release's `tag_name` (a leading `v` is dropped) with `VERSION`. `UPDATE_REPO` (`owner/name`) is set once, in `version.pri`; the about tab's GitHub link and the "no release yet" message use it too, so the rename (M16) changes that one line. Drafts and pre-releases are not "latest" on GitHub, so they are never offered. Until the repository has a published release, "Check now" in the options shows "No release has been published yet".
