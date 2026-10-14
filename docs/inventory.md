# Inventory

Mechanical survey of the codebase as of `master` at `dbdb12e` (2026-10-14). Findings only; no recommendations. File and line references are to that commit, and to the submodules at their pinned commits.

How it was gathered: reading `lightscreen.pro`, the `.pri` files and every source file; `grep` for `#include` lines, Qt class names and Win32/X11 calls; `git ls-remote` against upstream repositories for version data. Nothing was built for this survey. Items marked *(not verified here)* come from published sources or Stage 1 and were not re-checked in this session.

## 1. Size and history

- 40 first-party C++ files (20 `.cpp`, 20 `.h`), 7,446 lines, plus 7 `.ui` forms, 1 `.qrc`, 46 resource entries.
- 2 submodules, 816 lines of C++ in UGlobalHotkey and 274 in SingleApplication.
- 65 commits on `master`: 38 in 2016, 16 in 2017, 2 in 2018, 1 in 2019, 1 in 2020, 5 in 2021 (last upstream commit 2021-05-01, tag `v2.5`), 2 in 2026 (this fork).
- `VERSION = 2.5` in `version.pri`.

## 2. Dependencies

### Toolchain

| Item | Used | Latest | Status |
|---|---|---|---|
| Qt | 5.15.2 (Stage 1 build) | Qt 6.12.0 (tag on `qt/qtbase`); Qt 5 open-source line at `v5.15.19-lts-lgpl` | Qt 5.15 is the last Qt 5 release. Standard commercial support ended May 2025 *(not verified here)*. Qt 5.15.2 is the last open-source 5.15 binary release; later 5.15.x exist as LGPL source tags only. |
| C++ standard | C++14 (`CONFIG += c++14`, `lightscreen.pro:56`) | C++23 | — |
| Build system | qmake | — | Qt 6 still ships qmake; CMake is Qt's primary build system since Qt 6.0 *(not verified here)*. |
| Compiler (Windows) | MSVC 2022 with the `msvc2019_64` Qt kit (Stage 1) | — | — |
| Compiler (Linux) | not built yet | — | — |

### Qt modules

From `lightscreen.pro:58`, `:64` and `:91`, and the submodule `.pri` files.

| Module | Where | Used for | In Qt 6 |
|---|---|---|---|
| core, gui, widgets | all | — | present |
| network | uploader, updater, widgets, SingleApplication | HTTP (`QNetworkAccessManager`, `QHttpMultiPart`), `QLocalServer`/`QLocalSocket` | present |
| sql (+ SQLite driver) | `tools/screenshotmanager.cpp`, `dialogs/historydialog.cpp` | screenshot history database | present |
| multimedia | `lightscreenwindow.cpp:34,210-215` | `QSound::play` only | present, but `QSound` removed |
| concurrent | `dialogs/optionsdialog.cpp:37,625` | one `QtConcurrent::run` (SSL check) | present |
| winextras (Windows only) | `main.cpp`, `lightscreenwindow.*`, `tools/os.cpp`, `tools/windowpicker.cpp` | taskbar button and progress, jump list, DWM blur, HBITMAP/HICON conversion | removed in 6.0 |
| x11extras (Linux only) | `tools/os.cpp`, `tools/screenshot.cpp`, `tools/windowpicker.cpp` | `QX11Info::display()`, `appRootWindow()` | removed in 6.0 |
| gui-private (Linux only, via UGlobalHotkey) | `tools/UGlobalHotkey/uglobalhotkey-sources.pri`, `uglobalhotkeys.cpp:6,21` | `QPlatformNativeInterface` to get the xcb connection | private API, no compatibility guarantee |

### Submodules

| Submodule | Pinned | Origin | Upstream state | License |
|---|---|---|---|---|
| `tools/SingleApplication` → `ckaiser/SingleApplication` | `c6378ee`, 2016-06-07 | Fork of `itay-grudev/SingleApplication` v2.x, with a custom `instanceArguments` signal and Windows mutex (`README.md`) | `ckaiser/SingleApplication`: pinned commit is its `master` head, no commits since 2016. `itay-grudev/SingleApplication`: latest tag `v3.5.6`, last commit 2026-06-16, maintained, supports Qt 5 and 6 *(Qt 6 support per its README, not verified here)*. | MIT (`LICENSE`) |
| `tools/UGlobalHotkey` → `ckaiser/UGlobalHotkey` | `231b101`, 2016-08-07 | Fork of `falceeffect/UGlobalHotkey`, itself extracted from `bakwc/Pastexen` (`README.md`) | `ckaiser/UGlobalHotkey`: pinned commit is its `master` head, no commits since 2016. `falceeffect/UGlobalHotkey`: not reachable anonymously (`git ls-remote` asks for credentials, which is what GitHub does for a deleted or private repo). `bakwc/Pastexen`: last commit 2016-01-01. | Public domain per `README.md`; no `LICENSE` file |

Lightscreen uses these SingleApplication features: `SingleApplication` constructor and `instanceArguments` signal (`main.cpp:43,89`). UGlobalHotkey: `UGlobalHotkeys` (`lightscreenwindow.cpp:53`), `UKeySequence`.

### System libraries and external programs

| Dependency | Platform | Where |
|---|---|---|
| gdi32, user32, ole32, shell32, shlwapi, comctl32 | Windows | `lightscreen.pro:76` (MSVC), `:70` (MinGW) |
| user32 | Windows | `UGlobalHotkey/uglobalhotkey-libs.pri` |
| libX11 | Linux | `lightscreen.pro:90` |
| libxcb, libxcb-keysyms | Linux | `UGlobalHotkey/uglobalhotkey-libs.pri:3` |
| Carbon framework | macOS | `UGlobalHotkey/uglobalhotkey-libs.pri:4` (the app itself has no macOS build branch) |
| `optipng` (optional, run as a process) | Windows: `optipng.exe` next to the exe; Linux: on `PATH` | `tools/screenshot.cpp:170-190`, `dialogs/optionsdialog.cpp:228-238` |

### Online services

| Service | Where | Notes |
|---|---|---|
| Imgur API v3 (`api.imgur.com`) | `tools/uploader/imguruploader.cpp:41,64,81`, `widgets/imguroptionswidget.cpp:64,102` | OAuth client id and secret are string literals in `imguruploader.cpp:15-23`. |
| pomf-compatible upload hosts | `tools/uploader/pomfuploader.cpp`, `widgets/pomfoptionswidget.cpp` | Host list fetched from `https://lightscreen.com.ar/pomf.json` (`pomfoptionswidget.cpp:92`). |
| `lightscreen.com.ar` | `updater/updater.cpp:44` (version check), `lightscreenwindow.cpp:765`, `dialogs/updaterdialog.cpp:50` | Original author's site. Not reachable from this session (outbound blocked), so its current state is unknown. |

## 3. Module map

Line counts are `.cpp` + `.h`.

| Module | Files | Lines | Role |
|---|---|---|---|
| app | `main.cpp`, `lightscreenwindow.{cpp,h,ui}` | 1,275 | Entry point, main window, tray icon, hotkey handling, settings access |
| dialogs | `areadialog`, `historydialog`, `namingdialog`, `optionsdialog`, `previewdialog`, `updaterdialog` | 2,918 | Area selection overlay, history browser, file naming, options, preview, update prompt |
| tools | `os`, `screenshot`, `screenshotmanager`, `windowpicker` | 1,802 | Platform helpers, capture and save, history DB and screenshot list, window picker |
| tools/uploader | `uploader`, `imageuploader`, `imguruploader`, `pomfuploader` | 756 | Upload queue and the two upload back ends |
| updater | `updater` | 110 | Version check |
| widgets | `hotkeywidget`, `imguroptionswidget`, `pomfoptionswidget` | 585 | Custom widgets used by `optionsdialog.ui` |
| tools/SingleApplication | submodule | 274 | Single-instance guard |
| tools/UGlobalHotkey | submodule | 816 | Global hotkeys (Win32, xcb, Carbon) |

Per-file sizes over 500 lines: `lightscreenwindow.cpp` 1,027; `dialogs/optionsdialog.cpp` 867; `dialogs/areadialog.cpp` 757; `tools/screenshot.cpp` 525.

`dialogs/aboutdialog.ui` is in the tree but not in `FORMS` and not referenced anywhere. `lightscreen.pro:95` lists `TODO.txt` in `OTHER_FILES`; the file does not exist.

### Dependencies between files

From `#include` lines, plus the custom widgets declared in `dialogs/optionsdialog.ui`. Arrows mean "includes".

| From | To |
|---|---|
| `main.cpp` | `lightscreenwindow`, `tools/os`, SingleApplication |
| `lightscreenwindow` | `dialogs/optionsdialog`, `dialogs/previewdialog`, `dialogs/historydialog`, `tools/os`, `tools/screenshot`, `tools/screenshotmanager`, `tools/uploader/uploader`, `updater/updater`, UGlobalHotkey |
| `dialogs/areadialog` | `tools/os`, `tools/screenshot`, `tools/screenshotmanager` |
| `dialogs/historydialog` | `tools/os`, `tools/screenshotmanager`, `tools/uploader/uploader` |
| `dialogs/namingdialog` | `tools/os`, `tools/screenshot`, `tools/screenshotmanager` |
| `dialogs/optionsdialog` | `dialogs/namingdialog`, `dialogs/historydialog`, `tools/os`, `tools/screenshot`, `tools/screenshotmanager`, `tools/uploader/uploader`, `updater/updater`; via `.ui`: `widgets/hotkeywidget`, `widgets/imguroptionswidget`, `widgets/pomfoptionswidget` |
| `dialogs/previewdialog` | `tools/os`, `tools/screenshot`, `tools/screenshotmanager` |
| `dialogs/updaterdialog` | `tools/os` |
| `tools/os` | (none) |
| `tools/screenshot` | `tools/os`, `tools/screenshotmanager`, `tools/windowpicker`, `tools/uploader/uploader`, `dialogs/areadialog` |
| `tools/screenshotmanager` | `tools/os`, `tools/screenshot`, `tools/uploader/uploader` |
| `tools/windowpicker` | `tools/os` |
| `tools/uploader/uploader` | `imageuploader`, `imguruploader`, `tools/screenshotmanager` |
| `tools/uploader/imageuploader` | `imguruploader`, `pomfuploader`, `tools/os`, `tools/screenshotmanager` |
| `tools/uploader/imguruploader` | `imageuploader`, `uploader` |
| `tools/uploader/pomfuploader` | `imageuploader`, `uploader` |
| `updater/updater` | `dialogs/updaterdialog` |
| `widgets/hotkeywidget` | (none) |
| `widgets/imguroptionswidget` | `tools/screenshotmanager`, `tools/uploader/uploader`, `tools/uploader/imguruploader` |
| `widgets/pomfoptionswidget` | `tools/os`, `tools/screenshotmanager`, `tools/uploader/uploader`, `tools/uploader/pomfuploader` |

Cycles in that graph (file level):

- `tools/screenshot` ↔ `tools/screenshotmanager`
- `tools/screenshot` → `dialogs/areadialog` → `tools/screenshot` (crosses tools ↔ dialogs)
- `tools/screenshotmanager` → `tools/uploader/uploader` → `tools/screenshotmanager`
- `tools/uploader/uploader` ↔ `imguruploader`; `imageuploader` ↔ `imguruploader`; `imageuploader` ↔ `pomfuploader`
- `tools/uploader/imageuploader` → `tools/screenshotmanager` → `tools/uploader/uploader` → `imageuploader`

Module level: dialogs → tools, tools → dialogs; tools ↔ tools/uploader; dialogs → updater → dialogs; widgets → tools/uploader.

Shared global state:

- Singletons: `ScreenshotManager::instance()` (`tools/screenshotmanager.h:41`) and `Uploader::instance()` (`tools/uploader/uploader.h:36`), called from 12 `.cpp` files (`lightscreenwindow.cpp` 17 times, `dialogs/historydialog.cpp` 12 times).
- Settings: one `QSettings` object owned by `ScreenshotManager` (`tools/screenshotmanager.cpp:37,41`; `config.ini` next to the exe in portable mode, else the platform default). `LightscreenWindow::settings()` and `OptionsDialog::settings()` return it; the dialogs call `ScreenshotManager::instance()->settings()` directly. Separate `QSettings` objects: settings export/import (`dialogs/optionsdialog.cpp:129,143`) and the Windows autostart registry key (`tools/os.cpp:267`).
- The history database is the default `QSqlDatabase` connection, opened in `tools/screenshotmanager.cpp:56` and read in `dialogs/historydialog.cpp:185-188`.

### Platform-specific code

| File | Windows (Win32 calls) | Linux (X11/xcb calls) |
|---|---|---|
| `tools/os.cpp` | BitBlt, CreateCompatibleBitmap, CreateCompatibleDC, DeleteDC, DeleteObject, GetCursorInfo, GetDC, GetDesktopWindow, GetIconInfo, GetSystemMetrics, GetWindowDC, GetWindowRect, IsZoomed, ReleaseDC, SHGetFolderPath, SelectObject, SetForegroundWindow, ShowWindow; registry autostart via QSettings | XFree, XGetWindowProperty, XInternAtom, XQueryPointer, XQueryTree; autostart via `~/.config/autostart` |
| `tools/windowpicker.cpp` | GetAncestor, GetClassLong, GetWindowText, WindowFromPoint | XFree, XFreeStringList, XGetTextProperty, XGetWindowProperty, XInternAtom |
| `tools/screenshot.cpp` | GetDesktopWindow, GetForegroundWindow | XGetInputFocus |
| `dialogs/optionsdialog.cpp` | `nativeEvent` for `VK_SNAPSHOT` (`:482-495`) | — |
| `UGlobalHotkey/uglobalhotkeys.cpp` | RegisterHotKey, UnregisterHotKey | xcb_grab_key, xcb_ungrab_key, xcb_key_symbols_* |
| `SingleApplication/singleapplication.cpp` | CreateMutex, GetLastError | — |

`#ifdef Q_OS_MACOS` appears once (`dialogs/namingdialog.cpp:82`); there is no macOS section in `lightscreen.pro`. `lightscreenwindow.cpp:1000-1010` is under `#ifdef Q_WS_MAC`, a Qt 4 macro Qt 5 never defines, so the block is never compiled (it also refers to an undeclared `e`).

## 4. Deprecated and removed APIs

Qt 5.15 is the reference. The "Qt 6" column comes from the Qt 6 porting documentation, not from a Qt 6 build, which has not been tried.

### Removed in Qt 6

| API | Occurrences | Qt 6 |
|---|---|---|
| `QDesktopWidget`, `QApplication::desktop()` | `#include` in `dialogs/areadialog.cpp:30`, `dialogs/optionsdialog.cpp:23`, `dialogs/previewdialog.cpp:26`, `lightscreenwindow.cpp:21`, `main.cpp:20`, `tools/os.cpp:22`, `tools/screenshot.cpp:22`, `tools/windowpicker.cpp:20`. Calls: `dialogs/areadialog.cpp:505,529,709,712`; `dialogs/optionsdialog.cpp:475`; `dialogs/previewdialog.cpp:350,353,357,362`; `lightscreenwindow.cpp:978`; `tools/screenshot.cpp:412,414,436,437,443`; `tools/windowpicker.cpp:114` | removed |
| Qt WinExtras: `QtWin::isCompositionEnabled`, `enableBlurBehindWindow`, `extendFrameIntoClientArea` | `lightscreenwindow.cpp:86-89` | removed, no Qt replacement |
| Qt WinExtras: `QWinTaskbarButton` and progress | `lightscreenwindow.cpp:39,83`, `lightscreenwindow.h:36,126` (plus dummy classes for non-Windows at `:128-150`) | removed, no Qt replacement |
| Qt WinExtras: `QWinJumpList`, `QWinJumpListCategory` | `main.cpp:24,51,60,66,71` | removed, no Qt replacement |
| Qt WinExtras: `QtWin::fromHBITMAP`, `QtWin::fromHICON` | `tools/os.cpp:45,88,90,237`; `tools/windowpicker.cpp:33,171` | removed; `QImage::fromHBITMAP`/`fromHICON` exist in Qt 6 |
| Qt X11Extras: `QX11Info` | `tools/os.cpp:54,324,330,346,368,372`; `tools/screenshot.cpp:44,382`; `tools/windowpicker.cpp:42,195,198,216,218,224,230` | removed |
| `QRegExp` | `dialogs/namingdialog.cpp:76`; `dialogs/optionsdialog.cpp:94`; `SingleApplication/singleapplication.cpp:121` | moved to Core5Compat |
| `QRegExpValidator` | `#include` only, `widgets/pomfoptionswidget.cpp:7` (class not used) | moved to Core5Compat |
| `QSound` | `lightscreenwindow.cpp:34,210,213,215` | removed from Qt Multimedia |
| `QDirModel` | `dialogs/optionsdialog.cpp:24,602` | removed |
| `QSysInfo::WindowsVersion`, `WV_*` | `main.cpp:50,55`; `lightscreenwindow.cpp:82,93`; `dialogs/optionsdialog.cpp:201`; `tools/uploader/imguruploader.cpp:113,194`; `tools/uploader/pomfuploader.cpp:39,126`; `widgets/imguroptionswidget.cpp:170`; `widgets/pomfoptionswidget.cpp:130`; `updater/updater.cpp:39` | removed |
| `QPixmap::grabWindow` (static) | `tools/os.cpp:243`; `tools/screenshot.cpp:384`; `tools/windowpicker.cpp:294` | removed (`QScreen::grabWindow` remains) |
| `QLayout::setMargin` | `dialogs/areadialog.cpp:89`; `dialogs/optionsdialog.cpp:78`; `dialogs/previewdialog.cpp:88,91,242,247`; `tools/windowpicker.cpp:83,88,94,101,108` | removed |
| `QDateTime::toTime_t` | `tools/screenshot.cpp:108,512` | removed |
| `qrand` | `tools/screenshot.cpp:512` | removed |
| `QString::sprintf` | `tools/screenshot.cpp:99` | removed |
| `Qt::MidButton` | `dialogs/areadialog.cpp:443` | removed |
| `QPalette::Background` | `main.cpp:53` | removed |
| `QProcess::startDetached(const QString &)` (single command string) | `dialogs/optionsdialog.cpp:234`; `lightscreenwindow.cpp:312` | removed |
| `QLabel::pixmap()` returning a pointer | `tools/windowpicker.cpp:243` (`if (!mWindowIcon->pixmap())`) | returns `QPixmap` by value |
| `nativeEvent`/`nativeEventFilter` with `long *result` | `dialogs/optionsdialog.cpp:484`, `optionsdialog.h:51`; `UGlobalHotkey/uglobalhotkeys.h:46,48`, `uglobalhotkeys.cpp:172,181` | signature uses `qintptr *` |
| `QPlatformNativeInterface` (private header `qpa/qplatformnativeinterface.h`) | `UGlobalHotkey/uglobalhotkeys.cpp:6,21` | private API with no compatibility promise; Qt 6 adds the public `QNativeInterface` |

### Deprecated in Qt 6 but still present

| API | Occurrences |
|---|---|
| `Qt::AA_EnableHighDpiScaling` (no effect in Qt 6) | `main.cpp:41` |
| `QStandardPaths::DataLocation` | `tools/screenshotmanager.cpp:43` |
| `qAsConst` (deprecated since 6.6) | `tools/screenshotmanager.cpp:205`, `tools/uploader/imageuploader.cpp:28`, `widgets/pomfoptionswidget.cpp:117`. A backport in `tools/os.h:68-80` is compiled only for Qt < 5.7, so it is dead code on 5.15. |
| `QMouseEvent::pos()` | `dialogs/areadialog.cpp:255,270,279,333,388,397,399,432,434,464`; `dialogs/previewdialog.cpp:412,434` |
| `QMouseEvent::globalX()`/`globalY()` | `tools/windowpicker.cpp:146,147,275,276` |
| `int + Qt::KeyboardModifiers` to build a key sequence | `widgets/hotkeywidget.cpp:124` |

### Older but not deprecated

- String-based `SIGNAL()`/`SLOT()` connections: `tools/os.cpp` (3), `tools/screenshot.cpp` (3), `updater/updater.cpp` (1), `SingleApplication/singleapplication.cpp` (1). Everything else uses pointer-to-member `connect`.
- `foreach`: `dialogs/areadialog.cpp:387,747`; `UGlobalHotkey/uglobalhotkeys.cpp:132,136`.
- `Q_NULLPTR`: `dialogs/historydialog.cpp:25-26`, `tools/windowpicker.cpp:49`, `SingleApplication/singleapplication.cpp:25`.

### Windows API and build flags

- `GetClassLong(..., GCL_HICON)` at `tools/windowpicker.cpp:168` (MSVC C4312, flagged in Stage 1). It returns a 32-bit `DWORD`, which is cast to `HICON`; on x64 that truncates a 64-bit handle. `GetClassLongPtr`/`GCLP_HICON` is the pointer-sized variant.
- Windows XP targeting in `lightscreen.pro:77-82`: `_ATL_XP_TARGETING`, `/D _USING_V110_SDK71`, `/SUBSYSTEM:WINDOWS,5.01`, `/SUBSYSTEM:CONSOLE,5.01`. VS 2022 ignores 5.01 with LNK4010 (Stage 1).
- Runtime checks for Windows XP and Server 2003: `dialogs/optionsdialog.cpp:201` (`> WV_XP`), and the six `sslErrors` handlers listed under `QSysInfo::WindowsVersion` above, which call `ignoreSslErrors()` when the version is `<= WV_2003`.
- `SHGetFolderPath` (`tools/os.cpp:157`): deprecated by Microsoft in favor of `SHGetKnownFolderPath` since Vista *(not verified here)*.

### Other compiler-flagged code

- `tools/screenshot.cpp:388-404`, `Screenshot::extension()` (MSVC C4715, flagged in Stage 1). The `switch` handles all four enumerators (`PNG`, `BMP`, `WEBP`, `JPEG`) and has no `return` after it. `mOptions.format` is set by casting an integer read from settings (`lightscreenwindow.cpp:524`: `(Screenshot::Format) settings()->value("file/format").toInt()`), so a value outside 0-3 reaches the end of the function without returning. Stage 1's note said WEBP was outside the `switch`; it is inside it (`:398-399`), and was only removed from the options UI.
- The other 12 Stage 1 warnings are `size_t` to `int` narrowing in UGlobalHotkey and at `lightscreenwindow.cpp:113` (Stage 1, `docs/build.md`).

### Other mechanical findings

- `lightscreenwindow.cpp:215` plays `sound/ls.error.wav` (non-Windows error sound). The repo has `sounds/`, and it contains only `ls.screenshot.wav`. Both sound paths are relative to the working directory, and the `.wav` is not in `lightscreen.qrc`.
- `dialogs/areadialog.{cpp,h}` carry an LGPL notice (based on KDE KSnapshot's `regiongrabber.cpp`); the project `LICENSE` is GPL v2. 12 files have no license header: `dialogs/historydialog.*`, all of `tools/uploader/*`, `widgets/imguroptionswidget.*`, `widgets/pomfoptionswidget.*`.
- `version.pri` sets company and copyright to the original author ("Christian Kaiser", "2008-2021"); `main.cpp:38` sets organization name `"K"`, which decides the settings and `QSharedMemory`/`QLocalServer` keys.

## 5. Test coverage

| Module | Tests |
|---|---|
| app | none |
| dialogs | none |
| tools | none |
| tools/uploader | none |
| updater | none |
| widgets | none |
| SingleApplication (submodule) | none at the pinned commit |
| UGlobalHotkey (submodule) | none |

No test framework is configured: no `QTest` usage, no test `.pro`, no `tests/` directory.

## 6. Build, CI and packaging

**Build.** One qmake project, `lightscreen.pro`, `TEMPLATE = app`, which includes `version.pri` and the two submodule `.pri` files. No CMake. Branches:

- `windows`: MSVC branch (`contains(QMAKE_CC, cl)`) verified in Stage 1 with VS 2022 and Qt 5.15.2 (`docs/build.md`). MinGW branch (`contains(QMAKE_CC, gcc)`) not tried; it uses `libgdi32`-style library names and assigns `QMAKE_CXXFLAGS =` rather than appending.
- `unix`: `-lX11`, `x11extras`; UGlobalHotkey adds `-lxcb -lxcb-keysyms` and `gui-private` under `linux`. Not built yet.
- No macOS branch.
- `DEFINES += QT_DEPRECATED_WARNINGS`; no `QT_DISABLE_DEPRECATED_BEFORE`. Stage 1 saw no deprecation warnings against 5.15.2.

**CI.** None in the repo. No `.github/workflows`, no `appveyor.yml`, no Coverity configuration, and none in this repo's git history. `README.md:2` shows badges for the original author's AppVeyor project (`ckaiser/lightscreen`) and Coverity Scan project 6066.

**Packaging.** No installer sources (`.iss`, `.nsi`, `.wxs`) in the repo or its history. `build.md` documents a manual `windeployqt` step. Windows resources: `RC_ICONS += images/LS.ico`, `CONFIG += embed_manifest_exe`, `/MANIFESTUAC:level='asInvoker'`. Linux: no `.desktop` file or AppStream data in the repo; the app writes a minimal autostart `.desktop` at runtime (`tools/os.cpp:282-288`). The updater asks `lightscreen.com.ar` for new versions; there is no release automation.

**Resources.** `lightscreen.qrc` lists 46 files: icons from `images/` and `images/inv/` (a variant set). `sounds/ls.screenshot.wav` is loaded from disk, not from resources.
