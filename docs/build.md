# Building Lightscreen

Status as of Stage 1 (2026-10-14): the Windows x64 release build compiles, links and launches on a current toolchain **with no source or build-file changes**. Since M01, both the Windows and the Linux build run in GitHub Actions on every push (see "Continuous integration" below). There are no tests yet (upstream never had any), so there is nothing to run beyond the build.

## Windows (verified)

### Toolchain

| Component | Version | Installed with |
|---|---|---|
| Visual Studio 2022 Build Tools, "Desktop development with C++" workload | 17.14 (MSVC 19.44.35229, x64) | `winget` |
| Qt | 5.15.2, `msvc2019_64` kit (binary-compatible with MSVC 2022) | `aqtinstall` 3.3.0 |
| Python (only to run aqtinstall) | 3.12 | already present |

Qt 5.15.2 is the last Qt 5 release with official open-source Windows binaries, and the code targets Qt 5 (it uses `winextras`, removed in Qt 6), so it is the version that needs the fewest changes. All required Qt modules (core, gui, widgets, network, sql, multimedia, concurrent, winextras) are in the base `msvc2019_64` package; no extra `-m` modules are needed.

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
sudo apt-get install qtbase5-dev qtbase5-private-dev qtmultimedia5-dev libqt5x11extras5-dev \
  libqt5sql5-sqlite libx11-dev libxcb-keysyms1-dev xvfb
mkdir build && cd build
qmake ../lightscreen.pro
make -j"$(nproc)"
```

`qtbase5-private-dev` is needed because UGlobalHotkey includes `qpa/qplatformnativeinterface.h`. The app needs an X display (real or `xvfb-run`); with `QT_QPA_PLATFORM=offscreen` it segfaults at startup.

## Continuous integration

`.github/workflows/ci.yml` runs on every push and pull request, with two jobs:

- **linux** (`ubuntu-latest`): installs the packages above, builds, then runs the *Linux smoke*. It writes a `config.ini` next to the binary (portable mode, so no user settings are touched) with `file/format`, `file/enabled` and `file/target` set so the first-run options dialog is skipped, `options/tray=false` so no system tray is needed, and `options/disableUpdater=true` so the run makes no network calls. Under `xvfb-run -a` it starts `./lightscreen`, waits two seconds, runs `./lightscreen --quit` (SingleApplication forwards it to the running instance), and fails unless the first process exits with status 0 within ten seconds.
- **windows** (`windows-latest`): Qt 5.15.2 `win64_msvc2019_64` from `jurplel/install-qt-action` (cached), MSVC x64 from `ilammy/msvc-dev-cmd`, `qmake CONFIG+=release` and `nmake`, then `windeployqt --release --no-translations`. The deploy folder is uploaded as the `lightscreen-win64` artifact on the run's page; unzip it and run `lightscreen.exe`.

To run the smoke locally after a Linux build, copy the script from the "Smoke test" step of the workflow into `build/smoke.sh` and run `xvfb-run -a bash smoke.sh` from `build/`. If a previous run was killed, a stray `lightscreen` process will make the next run forward its arguments and exit; kill it first.

## What broke and how it was fixed

Nothing needed fixing to build. Things noticed along the way, left unchanged on purpose:

- **Windows XP targeting flags are obsolete.** The MSVC branch of `lightscreen.pro` sets `_ATL_XP_TARGETING`, `/D _USING_V110_SDK71` and `/SUBSYSTEM:WINDOWS,5.01`. With VS 2022 the linker ignores the 5.01 version (`LNK4010: invalid subsystem version number 5.01; default subsystem version assumed`) and the other two are no-ops. Harmless for now; remove them when the minimum Windows version is decided in `docs/goals.md`.
- **The MinGW branch of `lightscreen.pro` looks unmaintained** (library names like `libgdi32`, and it overwrites `QMAKE_CXXFLAGS` instead of appending). Not tried.
- **README badges** pointed at the original author's AppVeyor and Coverity projects. Replaced by the fork's CI badge in M01.
- **Compiler warnings worth a look** (14 in total, the rest are `size_t` to `int` narrowing in UGlobalHotkey and `lightscreenwindow.cpp:113`):
  - `tools/windowpicker.cpp:168` (C4312): `GetClassLong(..., GCL_HICON)` returns a 32-bit value, which truncates the icon handle on 64-bit builds. `GetClassLongPtr` / `GCLP_HICON` is the 64-bit-safe call. Likely a real bug in the window picker's icon on x64.
  - `tools/screenshot.cpp:404` (C4715): `Screenshot::extension()` has no return for formats outside its `switch` (WEBP was removed from the UI but is still in the enum). Undefined behavior if it is ever hit.
- **`QT_DEPRECATED_WARNINGS`** is defined, but against Qt 5.15.2 the build emits no C4996 deprecation warnings. Qt 6 porting issues (`winextras`, `x11extras`, `QRegExp`, `QDesktopWidget`) will only show up when building against Qt 6.

## Known flaky tests

None: there are no tests yet.

## Third-party libraries

SingleApplication and UGlobalHotkey used to be git submodules pointing at the original author's repos. They are now vendored (copied unchanged) under `tools/SingleApplication` and `tools/UGlobalHotkey`, so a plain clone builds with no `git submodule` step. Each directory has a `VENDORED.md` with its origin and pinned commit. The reasoning is in `docs/decisions.md`.
