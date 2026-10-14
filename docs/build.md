# Building Lightscreen

Status as of Stage 1 (2026-10-14): the Windows x64 release build compiles, links and launches on a current toolchain **with no source or build-file changes**. There are no tests yet (upstream never had any), so there is nothing to run beyond the build.

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
git submodule update --init
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

## Linux (not verified)

The `.pro` file supports Linux via X11 (`LIBS += -lX11`, `QT += x11extras`). It has not been built in this stage. On Debian/Ubuntu the expected dependencies are `qtbase5-dev qtmultimedia5-dev libqt5x11extras5-dev libqt5sql5-sqlite libx11-dev libxcb-keysyms1-dev build-essential`, then `qmake && make`. Verifying this, and putting the setup commands into the cloud environment configuration, is still open.

## What broke and how it was fixed

Nothing needed fixing to build. Things noticed along the way, left unchanged on purpose:

- **Windows XP targeting flags are obsolete.** The MSVC branch of `lightscreen.pro` sets `_ATL_XP_TARGETING`, `/D _USING_V110_SDK71` and `/SUBSYSTEM:WINDOWS,5.01`. With VS 2022 the linker ignores the 5.01 version (`LNK4010: invalid subsystem version number 5.01; default subsystem version assumed`) and the other two are no-ops. Harmless for now; remove them when the minimum Windows version is decided in `docs/goals.md`.
- **The MinGW branch of `lightscreen.pro` looks unmaintained** (library names like `libgdi32`, and it overwrites `QMAKE_CXXFLAGS` instead of appending). Not tried.
- **README badges** point at the original author's AppVeyor and Coverity projects. There is no CI for the fork yet.
- **Compiler warnings worth a look** (14 in total, the rest are `size_t` to `int` narrowing in UGlobalHotkey and `lightscreenwindow.cpp:113`):
  - `tools/windowpicker.cpp:168` (C4312): `GetClassLong(..., GCL_HICON)` returns a 32-bit value, which truncates the icon handle on 64-bit builds. `GetClassLongPtr` / `GCLP_HICON` is the 64-bit-safe call. Likely a real bug in the window picker's icon on x64.
  - `tools/screenshot.cpp:404` (C4715): `Screenshot::extension()` has no return for formats outside its `switch` (WEBP was removed from the UI but is still in the enum). Undefined behavior if it is ever hit.
- **`QT_DEPRECATED_WARNINGS`** is defined, but against Qt 5.15.2 the build emits no C4996 deprecation warnings. Qt 6 porting issues (`winextras`, `x11extras`, `QRegExp`, `QDesktopWidget`) will only show up when building against Qt 6.

## Known flaky tests

None: there are no tests yet.

## Submodules: recommendation

Both submodules point at the original author's own forks, which are as unmaintained as the app:

| Submodule | Pinned commit | Size | Upstream status |
|---|---|---|---|
| `tools/SingleApplication` → `ckaiser/SingleApplication` | `c6378ee` (2016-06-07) | ~300 lines incl. header | Actively maintained upstream at `itay-grudev/SingleApplication` (supports Qt 5 and 6). The author's copy is based on an old v2.x with custom API (`instanceArguments` signal, `createMutex`) that `main.cpp` uses, so switching means code changes. |
| `tools/UGlobalHotkey` → `ckaiser/UGlobalHotkey` | `231b101` (2016-08-07) | ~800 lines incl. headers | Original (`falceeffect/UGlobalHotkey`) is also unmaintained. No obvious maintained drop-in; the Qt 6 port will need changes either way (its Linux side uses X11/xcb). |

**Recommendation: vendor both** (copy their sources into the repo, keep their LICENSE files and a note on the origin commit, drop `.gitmodules`). They are small, already diverge from any upstream, and the fork should not depend on someone else's dormant repos for every clone. Revisit SingleApplication as part of the Qt 6 decision: moving to the maintained `itay-grudev` version is the natural time to take the API change.

Alternatives considered: forking both under the fork's account (keeps history but adds two more repos to maintain for ~1,100 lines), or switching SingleApplication now (a code change, which Stage 1 avoids).

Not changed in Stage 1; needs your decision.
