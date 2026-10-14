# Modernization plan

Stage 3 output (2026-10-14). A staged plan for the Lightscreen fork that keeps the code building and working after every step. Written from `master` at `d92b3ca` (after PR #4), `docs/goals.md`, `docs/inventory.md`, `docs/known-bugs.md` and a full read of the source. Nothing was built for this document: the cloud container has no Qt installed, so every claim about an API below comes from the code and the Qt documentation, not from a compile.

Decisions behind the plan are in `docs/decisions.md` (entries dated 2026-10-14, Stage 3). The two questions it raised for the owner were answered in the Stage 3 thread; section 8 records the answers.

## 1. How to read this plan

- **A step is one PR.** Each step lists its tag, the steps it depends on, the files it touches, what it does, and a done-check. A step does not start until its dependencies are merged to `master`.
- **Tags.** *Mechanical* steps have a complete spec in this file and change no behaviour that a safety-net test records (or say exactly which test changes and why). *Structural* steps change module boundaries or behaviour and need judgment. Per `docs/PLAN.md`, the mechanical round runs with Sonnet-class workers and the structural round with Opus-class workers.
- **Merge policy** (owner decision, 2026-10-14): a thread merges its own PR when it is docs-only or mechanical and verified by a build and the tests. The owner merges anything that changes behaviour, touches releases or tags, or where the thread is unsure. Each step below says who merges.
- **Done-check vocabulary.**
  - *tests pass*: all safety-net tests green on both CI platforms (Linux under `xvfb-run`, Windows with `QT_QPA_PLATFORM=offscreen`).
  - *CI green*: both GitHub Actions jobs (ubuntu, windows-latest) pass on the PR head.
  - *Linux smoke*: the CI check that starts the app under Xvfb in portable mode and stops it with a second `--quit` instance (M01).
  - *Windows build*: the windows-latest job compiles and links the MSVC build. This needs no one's machine.
  - *Windows QA*: a manual check on the owner's machine through Remote Control. Cloud threads cannot do this. Each step that needs it names the exact things to try, so the session is short.
  - *include-graph check*: a script (added in S01) that fails CI if a file under `tools/` includes anything under `dialogs/` or `widgets/`, or if `tools/uploader/` includes anything outside itself and Qt.
- **Where the work runs.** Cloud threads run in Linux containers with no display: they can build the Linux target, run the tests under Xvfb, and get a Windows *build* from CI. Anything that must be seen or clicked on Windows is Windows QA.

## 2. Timeline

```
Round 0 (now, before Stage 4)   M01 CI  ->  M02 test harness and seams  (M14 housekeeping any time)
Stage 4 safety-net tests         one session per module, order in section 6
Stage 5 known bugs               one session per "fix now" bug, section 7
Mechanical round (Sonnet)        M03 -> M12 -> M13 -> M04 -> M05 -> M06 -> M07 -> M08 -> M09 -> M10 -> M11 -> M15
Structural round (Opus)          S01 -> S02 -> {S03, S04, S05} -> S06 (optional)
Last                             M16 rename  ->  first tagged release (Stage 7)
```

M01 and M02 are plan steps that must land *before* Stage 4, because the tests need a harness and seams to exist. They are mechanical and small.

## 3. Mechanical steps

### M01. GitHub Actions CI on Windows and Linux

- **Tag:** mechanical. **Depends on:** nothing. **Merges:** thread.
- **Touches:** `.github/workflows/ci.yml` (new), `docs/build.md`, `README.md` (replace the upstream badges with the fork's).
- **What.** One workflow, two jobs, on push and pull request:
  - `ubuntu-latest`: `apt-get install qtbase5-dev qtbase5-private-dev qtmultimedia5-dev libqt5x11extras5-dev libqt5sql5-sqlite libx11-dev libxcb-keysyms1-dev xvfb`; `qmake && make -j`; then the Linux smoke: write a `config.ini` next to the binary containing at least `[file]\nformat=0\nenabled=true\ntarget=<tmpdir>` and `[options]\ntray=false` (so the first run neither opens the options dialog nor needs a tray), start `./lightscreen` under `xvfb-run -a`, wait two seconds, run `./lightscreen --quit` (SingleApplication forwards it to the first instance), and assert the first process exits within ten seconds with status 0.
  - `windows-latest`: `jurplel/install-qt-action` (or `aqtinstall` directly) with Qt 5.15.2 `win64_msvc2019_64`; `ilammy/msvc-dev-cmd` for `vcvars64`; `qmake CONFIG+=release && nmake`; `windeployqt --release --no-translations`; upload the deploy folder as a zip artifact.
  - Pin action versions by tag. Cache the Qt install on Windows (the action supports it).
- **Done-check.** CI green. Linux smoke passes. The Windows artifact downloads and starts on the owner's machine (one-time Windows QA, optional: the Stage 1 build already proved the toolchain).
- **Why first.** Every later step's done-check says "CI green". It also gives cloud threads a Windows compile they do not have today.

### M02. Test harness and seams

- **Tag:** mechanical. **Depends on:** M01. **Merges:** thread.
- **Touches:** `lightscreen.pro`, `src.pri` (new), `tests/tests.pro` (new, `TEMPLATE = subdirs`), `tests/common.pri` (new), `tests/tst_smoke/` (new placeholder test), `tools/screenshotmanager.{h,cpp}`, `tools/uploader/uploader.{h,cpp}`, `tools/screenshot.{h,cpp}`, `.github/workflows/ci.yml`, `docs/build.md`, `.gitignore`.
- **What.**
  1. Move `HEADERS`, `SOURCES`, `FORMS`, `RESOURCES`, `QT +=`, the platform `LIBS` and the two vendored `.pri` includes from `lightscreen.pro` into `src.pri`. `lightscreen.pro` keeps `TEMPLATE = app`, `TARGET`, `main.cpp`, the Windows resource and manifest lines, and `include(src.pri)`. Each test `.pro` includes `../common.pri`, which includes `../../src.pri` and adds `QT += testlib`, `CONFIG += testcase`, `INCLUDEPATH += ../..`. Tests therefore compile the app's sources directly (no library), which is the least change for qmake.
  2. Seams, each a few lines and each keeping the existing call sites working:
     - `ScreenshotManager`: a second constructor `ScreenshotManager(QSettings *settings, const QString &historyPath, QObject *parent)`; the existing constructor computes its values as today and delegates. Add `static void setInstance(ScreenshotManager *)`. Tests build a manager on a temporary directory and install it.
     - `Uploader`: `static void setInstance(Uploader *)` and `void setNetwork(QNetworkAccessManager *)`. Tests pass a `QNetworkAccessManager` subclass whose `createRequest` returns a canned `QNetworkReply`, the standard QtTest pattern.
     - `Screenshot`: `static QString extensionFor(Format)` (public; the private `extension()` calls it) and `static QString uniqueName(const QString &nameWithoutExtension, const QString &extension)` extracted verbatim from the collision loop in `save()`, quirks included. Both are needed by the Stage 4 naming tests and the bug 2 fix.
  3. CI runs `make check` in the tests directory on Linux under `xvfb-run -a` and on Windows with `QT_QPA_PLATFORM=offscreen` (QtTest widget tests work offscreen; only `LightscreenWindow`, which constructs `UGlobalHotkeys`, needs a real X server).
  4. `docs/build.md`: how to build and run the tests on both platforms.
- **Done-check.** CI green including the placeholder test on both platforms. The app builds and the Linux smoke still passes. Windows build.
- **Note.** `UGlobalHotkeys` dereferences the xcb connection in its constructor, which is why the app segfaults with `QT_QPA_PLATFORM=offscreen` (`docs/status.md`). Tests that construct `LightscreenWindow` must run under Xvfb; everything else can run offscreen.

*Stage 4 and Stage 5 run here. See sections 5 and 6.*

### M03. Windows 10 minimum: drop pre-Windows-10 code and the XP build flags

- **Tag:** mechanical. **Depends on:** M02 and the Stage 4 tests. **Merges:** thread.
- **Touches:** `lightscreen.pro`, `main.cpp`, `lightscreenwindow.cpp`, `dialogs/optionsdialog.cpp`, `dialogs/namingdialog.cpp`, `tools/os.cpp`, `tools/uploader/imguruploader.cpp`, `tools/uploader/pomfuploader.cpp`, `widgets/imguroptionswidget.cpp`, `widgets/pomfoptionswidget.cpp`, `updater/updater.cpp`, `docs/build.md`.
- **What.**
  - `lightscreen.pro`: remove `_ATL_XP_TARGETING`, both `_USING_V110_SDK71` lines, both `/SUBSYSTEM:...,5.01` lines (keep `/MANIFESTUAC`). Remove the MinGW branch (`contains(QMAKE_CC, gcc)`): it is unmaintained, untested, and CI builds with MSVC (`docs/decisions.md`).
  - Every `QSysInfo::WindowsVersion` comparison goes, taking the branch that Windows 10 takes: `main.cpp:50,55` (jump list always created; tooltip colour always black on Windows 10, so use `Qt::black`), `lightscreenwindow.cpp:82,93` (taskbar button always; the pre-Windows-7 stylesheet block deleted), `dialogs/optionsdialog.cpp:201` (cursor default `true`), the six `sslErrors` handlers that call `ignoreSslErrors()` (delete the handlers entirely; the `PomfUploader::verify` one keeps only `callback(false)`), `updater/updater.cpp:39` (`platform = QSysInfo::productType() + "_" + QSysInfo::productVersion()` on all platforms).
  - `lightscreenwindow.cpp:1000-1010`: delete the `#ifdef Q_WS_MAC` block (bug 8). `dialogs/namingdialog.cpp:82`: delete the `Q_OS_MACOS` block (macOS is not a target).
  - `tools/os.cpp:157`: `SHGetFolderPath(CSIDL_PERSONAL)` becomes `SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_CREATE, ...)` with `CoTaskMemFree`.
- **Done-check.** Tests pass. CI green. The Windows job log no longer shows `LNK4010`. Closes bugs 7 and 8.

### M12. Remove Imgur

Numbered M12 for historical reasons but it runs right after M03, so that no dead code is ported in M04 to M09.

- **Tag:** mechanical (a deletion with a complete spec). **Depends on:** M03. **Merges:** owner (behaviour change, although one already decided).
- **Touches:** delete `tools/uploader/imguruploader.{h,cpp}`, `widgets/imguroptionswidget.{h,cpp,ui}`; `src.pri`; `dialogs/optionsdialog.{cpp,ui}`; `lightscreenwindow.cpp`; `lightscreen.qrc`; `dialogs/historydialog.cpp`; `tools/uploader/imageuploader.cpp`; `tools/uploader/uploader.cpp`; `tools/screenshotmanager.cpp`; `tests/` (settings round-trip expectations).
- **What.**
  - Options dialog, upload tab: the service combo box keeps one entry (the pomf uploader, until S03 replaces it); the Imgur page leaves the stacked widget; the `beginGroup("imgur")` blocks in `loadSettings`/`saveSettings` go.
  - `Uploader::serviceName`: index 0 and the default return `"pomf"`. `ImageUploader::factory` drops the Imgur branch and returns the pomf uploader for anything it does not recognise instead of `nullptr` (today a null pointer reaches `connect`).
  - `ScreenshotManager::saveHistory`/`updateHistory`: store the third argument as the delete URL as given, without the `https://imgur.com/delete/` prefix. The column keeps its name; existing rows keep their values.
  - `HistoryDialog`: "Delete from imgur.com" becomes "Open delete link", enabled only when the row's delete URL is non-empty. Fix the settings key in `HistoryDialog::upload` from `options/upload/service` to `upload/service` and pass it through `Uploader::serviceName` (bug 14, below).
  - Menus and tooltips in `lightscreenwindow.cpp` lose "imgur.com"; the `imgur` icon alias in `lightscreen.qrc` is renamed `upload` (same PNG) and every `os::icon("imgur")` follows.
  - Settings: the `upload/imgur/*` keys are left alone (harmless, and a user who downgrades keeps their token). `upload/service` values above the new range read as 0.
- **Done-check.** Tests pass, with the settings round-trip expectation updated in the same PR and the diff explained in the PR body. CI green. `grep -ri imgur` finds nothing outside `docs/` and git history. Windows QA optional.

### M13. Update check via GitHub Releases; no more `lightscreen.com.ar`

- **Tag:** mechanical (a rewrite, but with a complete spec and a headless test). **Depends on:** M12, M02. **Merges:** owner (behaviour).
- **Touches:** `updater/updater.{h,cpp}`, `dialogs/updaterdialog.{h,cpp}`, `lightscreenwindow.cpp` (`updaterDone`), `dialogs/optionsdialog.ui` (SSL help link, home-page link), `widgets/pomfoptionswidget.{cpp,ui}` ("download list" button and the `pomf.json` fetch go), `version.pri` (`DEFINES += UPDATE_REPO=\"Ogrueb/Lightscreen\"`), `tests/tst_updater/` (new), `docs/build.md`.
- **What.** `Updater::check()` requests `https://api.github.com/repos/$UPDATE_REPO/releases/latest` with headers `Accept: application/vnd.github+json` and a `User-Agent` naming the app and version. On success it reads `tag_name`, strips a leading `v`, and compares with `QVersionNumber` against `qApp->applicationVersion()`. It emits `done(bool newer, QString htmlUrl)` on success and `failed(QString message)` on any network error, non-200 status, or JSON without `tag_name`. The updater takes its `QNetworkAccessManager` from `Uploader::network()` (seam from M02) so the test can stub it. `UpdaterDialog` shows the error text on `failed`; the silent weekly check does nothing on `failed` except record the attempt (it keeps the seven-day throttle, so a dead network does not retry every start). `updaterDone`'s "see more" opens `htmlUrl`.
- **Done-check.** Unit test: newer tag → `done(true)`; equal tag → `done(false)`; `v`-prefixed tag parses; HTTP 404 → `failed`; malformed body → `failed`; network error → `failed`. CI green. `grep -r lightscreen.com.ar` finds nothing outside `docs/`. Windows QA (optional): "Check now" in the options dialog shows an error until the fork has a release, which is the correct result. Closes bug 6.

### M04. Replace the removed-in-Qt-6 APIs that already have Qt 5.15 equivalents

- **Tag:** mechanical. **Depends on:** M13. **Merges:** thread.
- **Touches:** `dialogs/areadialog.cpp`, `dialogs/namingdialog.cpp`, `dialogs/optionsdialog.cpp`, `dialogs/previewdialog.cpp`, `dialogs/historydialog.{h,cpp}`, `lightscreenwindow.cpp`, `main.cpp`, `tools/os.{h,cpp}`, `tools/screenshot.cpp`, `tools/screenshotmanager.cpp`, `tools/windowpicker.cpp`, `tools/uploader/imageuploader.cpp`, `tools/uploader/pomfuploader.cpp`, `widgets/pomfoptionswidget.cpp`, `tools/SingleApplication/singleapplication.cpp`, `tools/UGlobalHotkey/uglobalhotkeys.cpp`, `updater/updater.cpp`.
- **What.** Line references are in `docs/inventory.md` section 4. One replacement per row; no other edits.

  | Old | New |
  |---|---|
  | `QRegExp` (`namingdialog.cpp:76`, `optionsdialog.cpp:94`, `singleapplication.cpp:121`) | `QRegularExpression`; `exactMatch` becomes `match().hasMatch()` on an anchored pattern |
  | `#include <QRegExpValidator>` (`pomfoptionswidget.cpp:7`) | delete the include |
  | `QDirModel` (`optionsdialog.cpp:602`) | `QFileSystemModel` with `setFilter(QDir::Dirs \| QDir::NoDotAndDotDot)` and `setRootPath(QString())` |
  | `QDateTime::toTime_t()` (`screenshot.cpp:108,512`) | `toSecsSinceEpoch()` |
  | `qrand()` (`screenshot.cpp:512`) | `QRandomGenerator::global()->generate()` |
  | `QString().sprintf(fmt, n)` (`screenshot.cpp:99`) | `QString::asprintf(fmt, n)` |
  | `Qt::MidButton` (`areadialog.cpp:443`) | `Qt::MiddleButton` |
  | `QPalette::Background` (`main.cpp:53`) | `QPalette::Window` |
  | `QProcess::startDetached("explorer /select, \"" + f + "\"")` (`lightscreenwindow.cpp:312`) | `QProcess::startDetached("explorer", {"/select,", QDir::toNativeSeparators(f)})` |
  | `QProcess::startDetached("optipng")` as an existence test (`optionsdialog.cpp:234`) | `QStandardPaths::findExecutable("optipng").isEmpty()`. This also stops the options dialog from launching a stray `optipng` process on Linux |
  | `QLayout::setMargin(n)` (eleven sites) | `setContentsMargins(n, n, n, n)` |
  | `QPixmap::grabWindow(wid)` static (`os.cpp:243`, `screenshot.cpp:384`, `windowpicker.cpp:294`) | `QGuiApplication::primaryScreen()->grabWindow(wid)` (what the static did internally) |
  | `QMouseEvent::pos()` / `globalX()` / `globalY()` | `position().toPoint()` / `globalPosition().toPoint().x()` / `.y()` |
  | `QLabel::pixmap()` as a pointer (`windowpicker.cpp:243`) | `!mWindowIcon->pixmap(Qt::ReturnByValue).isNull()` |
  | `QStandardPaths::DataLocation` (`screenshotmanager.cpp:43`) | `AppLocalDataLocation` (same value, so the history path does not move) |
  | `qAsConst` backport in `os.h:68-80` | delete; use `qAsConst` from Qt (replaced by `std::as_const` in M10) |
  | `foreach` (`areadialog.cpp:387,747`, `uglobalhotkeys.cpp:132,136`) | range-based `for` |
  | `Q_NULLPTR` | `nullptr` |
  | string `SIGNAL()`/`SLOT()` connects (`os.cpp` ×3, `screenshot.cpp` ×3, `updater.cpp` ×1, `singleapplication.cpp` ×1) | pointer-to-member connects; `QProcess::finished` needs `QOverload<int, QProcess::ExitStatus>::of`. The `os::effect` helper takes `const char *slot`: keep its signature, it is a string-slot API by design, and leave its three connects as they are |

  Left for M09 because Qt 5.15 has no equivalent: `nativeEvent`/`nativeEventFilter` `long *` signatures, `event->key() + modifiers` in `hotkeywidget.cpp:124`, `Qt::AA_EnableHighDpiScaling`, and everything under winextras and x11extras (M07, M08).
- **Done-check.** Tests pass. The naming tests (leading zeros, timestamp naming, `uniqueName`) prove the `asprintf`, `toSecsSinceEpoch` and `QRandomGenerator` swaps kept their output shape. CI green. Linux smoke.

### M05. `QDesktopWidget` to `QScreen`

- **Tag:** mechanical. **Depends on:** M04. **Merges:** owner (capture geometry is involved, even though the mapping is one-to-one).
- **Touches:** `dialogs/areadialog.cpp`, `dialogs/optionsdialog.cpp`, `dialogs/previewdialog.cpp`, `lightscreenwindow.cpp`, `main.cpp`, `tools/os.cpp`, `tools/screenshot.cpp`, `tools/windowpicker.cpp` (the eight files that include `<QDesktopWidget>`).
- **What.** One-to-one replacements, keeping today's arithmetic (the DPI composition is S04's job):

  | Old | New |
  |---|---|
  | `desktop()->screenNumber(pos)` then `screen(n)->geometry()` / `screenGeometry(n)` | `QScreen *s = QGuiApplication::screenAt(pos); if (!s) s = QGuiApplication::primaryScreen(); s->geometry()` |
  | `desktop()->screenGeometry(this)` / `availableGeometry(this)` | `this->screen()->geometry()` / `->availableGeometry()` (`QWidget::screen()` exists since 5.14) |
  | `desktop()->availableGeometry()` (no argument) | `QGuiApplication::primaryScreen()->availableGeometry()` |
  | `desktop()->geometry()` | `QGuiApplication::primaryScreen()->virtualGeometry()` |
  | `desktop()->screenGeometry(desktop()->primaryScreen())` | `QGuiApplication::primaryScreen()->geometry()` |
  | `desktop()->winId()` as the window to grab | `0` (`QScreen::grabWindow(0, ...)` grabs the whole screen in both Qt 5 and 6) |
  | `desktop()->devicePixelRatio()` | `QGuiApplication::primaryScreen()->devicePixelRatio()` |

- **Done-check.** Tests pass (the `AreaDialog::resultRect` test is unchanged). CI green. Linux smoke. **Windows QA:** whole screen, current monitor (cursor on the non-primary monitor: bug 11 re-test), area, active window, window picker; options dialog and preview appear on the expected monitor. Report any difference from before as a Stage 6 finding, not a blocker, unless something no longer works.

### M06. Sound without QtMultimedia

- **Tag:** mechanical. **Depends on:** M04. **Merges:** thread (Windows QA by the owner before the thread merges).
- **Touches:** `src.pri` (`QT -= multimedia`), `lightscreen.qrc` (add `sounds/ls.screenshot.wav` under `/sounds`), `tools/os.{h,cpp}` (`void os::playSound(os::Sound)` with `Screenshot` and `Error`), `lightscreenwindow.cpp:210-216`, `tests/` (a resource-exists test).
- **What.** Windows: `PlaySound` from `winmm` with `SND_MEMORY | SND_ASYNC` on the bytes of the qrc resource for the success sound, and `PlaySound(TEXT("SystemAsterisk"), nullptr, SND_ALIAS | SND_ASYNC)` for the error (the current code gets the default sound by passing a nonsense path; this is the documented way). Add `winmm.lib` to `LIBS`. Linux: no-op. The options dialog already hides the sound checkbox and forces it off on Linux (`optionsdialog.cpp:243-245`), so Linux behaviour is unchanged and bug 3 is closed by this step on the only platform where the option exists.
- **Done-check.** Tests pass, including a test that `:/sounds/ls.screenshot.wav` is a non-empty resource. CI green. The Windows deploy artifact no longer contains `Qt5Multimedia.dll`. **Windows QA:** enable "play sound", take a screenshot, hear it; take a cancelled area capture, hear the system sound.

### M07. Windows integration without QtWinExtras

- **Tag:** mechanical (reimplementation with a fixed spec; Windows QA required). **Depends on:** M05, M06 (both touch `lightscreenwindow.cpp` and `tools/os.cpp`). **Merges:** owner.
- **Touches:** `lightscreenwindow.{h,cpp}`, `main.cpp`, `tools/taskbar.{h,cpp}` (new), `tools/os.{h,cpp}`, `tools/windowpicker.cpp`, `src.pri`.
- **What.**
  - **Taskbar overlay and progress: kept.** New `Taskbar` class (Windows-only implementation, empty inline implementation elsewhere; replaces the `QWinTaskbarDummy` classes in `lightscreenwindow.h`) wrapping `ITaskbarList3`: `setOverlayIcon(QIcon)`, `clearOverlayIcon()`, `setProgressState(NoProgress | Indeterminate | Normal | Paused)`, `setProgressValue(int percent)`, `setWindow(QWindow *)`. It calls `CoCreateInstance(CLSID_TaskbarList)` lazily and `HrInit()`. The six call sites in `lightscreenwindow.cpp` map one to one (`setPaused(true)+setVisible(true)` = `Paused`, `resume()` = `Normal`, `setVisible(false)`/`stop()`/`reset()` = `NoProgress`).
  - **DWM blur and frame extension: dropped.** Delete `lightscreenwindow.cpp:86-90` (`isCompositionEnabled`, `WA_NoSystemBackground`, `enableBlurBehindWindow`, `extendFrameIntoClientArea`). On Windows 10 that call produces a transparent, not blurred, background; the main window gets the normal window background.
  - **Jump list: dropped** (owner decision; S07 can bring it back later). Delete the block in `main.cpp:49-78`.
  - `QtWin::fromHBITMAP`/`fromHICON` (`os.cpp:88,90,237`, `windowpicker.cpp:171`): route through `os::pixmapFromHBITMAP(HBITMAP, bool alpha)` and `os::pixmapFromHICON(HICON)`, which call `QtWin` under `#if QT_VERSION < QT_VERSION_CHECK(6,0,0)` and `QImage::fromHBITMAP`/`fromHICON` otherwise. `src.pri`: `lessThan(QT_MAJOR_VERSION, 6): QT += winextras` so the module is only linked on Qt 5. M10 removes the Qt 5 branch.
- **Done-check.** Tests pass. CI green (Windows build). **Windows QA:** upload a screenshot and watch the taskbar progress and the overlay icon; take a failed capture and see the "no" overlay; window picker shows window icons (with the Stage 5 fix for bug 1); the main window looks acceptable without the glass.

### M08. X11 without QtX11Extras

- **Tag:** mechanical. **Depends on:** M05. **Merges:** thread.
- **Touches:** `tools/os.{h,cpp}`, `tools/screenshot.cpp`, `tools/windowpicker.cpp`, `tools/UGlobalHotkey/uglobalhotkeys.{h,cpp}`, `tools/UGlobalHotkey/uglobalhotkey-sources.pri` (drop `gui-private`), `src.pri`, `.github/workflows/ci.yml` (apt list), `docs/build.md`.
- **What.** `os::x11Display()` returns `QX11Info::display()` on Qt 5 and `qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->display()` on Qt 6 (6.2+), behind `QT_VERSION` checks; `os::x11RootWindow()` returns `DefaultRootWindow(display)`. The thirteen `QX11Info` call sites use them. `UGlobalHotkeys` gets its `xcb_connection_t *` the same way (`->connection()` on Qt 6; `QX11Info::connection()` on Qt 5) instead of `QPlatformNativeInterface`, so the private `gui-private` module is no longer needed. `nativeEventFilter`'s result parameter gets a `typedef` that is `long` on Qt 5 and `qintptr` on Qt 6 (in the vendored header; `VENDORED.md` records the change). `src.pri`: `lessThan(QT_MAJOR_VERSION, 6): QT += x11extras`.
- **Done-check.** Tests pass. CI green. Linux smoke. Optional extension of the smoke test, if `xdotool` is installed in the runner: `xdotool key Print` under Xvfb produces a file in the portable screenshots directory, which exercises the hotkey path end to end.

### M09. The Qt 6 flip

- **Tag:** mechanical. **Depends on:** M07, M08. **Merges:** owner (full Windows QA pass).
- **Touches:** `.github/workflows/ci.yml`, `src.pri` (`CONFIG += c++17`), `dialogs/optionsdialog.{h,cpp}` and `tools/UGlobalHotkey/uglobalhotkeys.{h,cpp}` (`nativeEvent` with `qintptr *result`), `widgets/hotkeywidget.cpp` (`QKeySequence(QKeyCombination(modifiers, Qt::Key(key)))`), `tools/windowpicker.cpp` (`pixmap()` by value), `main.cpp` (remove `AA_EnableHighDpiScaling`), `tools/screenshotmanager.cpp` (`QSqlDatabase` unchanged; check `QSqlQuery` bind semantics), `tests/`, `docs/build.md`, `README.md`.
- **What.** CI installs Qt 6 with `aqtinstall` on both runners, the same version on both: the current Qt 6 LTS at the time this step runs (6.8.x unless a newer LTS exists), `win64_msvc2022_64` on Windows and `linux_desktop gcc_64` on Linux, with the `qtbase`, `qttools` and SQLite driver pieces (`qtbase` carries `sql` and the `QSQLITE` plugin). The Linux job still installs the X11/xcb development packages from apt. Then build, run the tests, run the smoke. Fix what the compiler finds; the expected list is the four items under "Left for M09" in M04 plus anything the `QT_DISABLE_DEPRECATED_BEFORE=0x060000` define surfaces (add that define in this step so later deprecations are errors). `docs/build.md` is rewritten for the Qt 6 toolchain: on Windows, `aqt install-qt windows desktop <ver> win64_msvc2022_64`; on Ubuntu 24.04, `qt6-base-dev qt6-base-private-dev libqt6sql6-sqlite` is enough for a local build (apt's 6.4 is older than CI's Qt but satisfies every API the code uses; CI's version is the reference).
- **Done-check.** CI green on Qt 6 on both platforms; tests pass; Linux smoke. **Windows QA**, the long one: install over a 2.5 configuration and confirm settings and history carry over (the compatibility commitment in `docs/goals.md`); all five capture modes; hotkeys incl. Print Screen in the hotkey widget; tray menu; options dialog incl. export/import; history dialog; preview dialog; portable mode with `config.ini`; autostart on and off. Anything broken is fixed in this PR before merge.

### M10. Purge the Qt 5 branches

- **Tag:** mechanical. **Depends on:** M09. **Merges:** thread.
- **Touches:** `tools/os.cpp`, `tools/windowpicker.cpp`, `tools/UGlobalHotkey/*`, `src.pri`, `.github/workflows/ci.yml` (nothing left to drop), plus `qAsConst` → `std::as_const` (three sites), `QMouseEvent` leftovers, `tools/UGlobalHotkey/VENDORED.md` and `tools/SingleApplication/VENDORED.md` (list the local modifications made since vendoring).
- **Done-check.** Tests pass, CI green, `grep -rn "QT_VERSION_CHECK" --include=*.cpp --include=*.h .` returns nothing.

### M11. CMake replaces qmake

- **Tag:** mechanical. **Depends on:** M10. **Merges:** thread, after the owner has built once with the new instructions (Windows QA: build only).
- **Touches:** `CMakeLists.txt` (root, new), `tests/CMakeLists.txt` (new), `tools/SingleApplication/CMakeLists.txt` and `tools/UGlobalHotkey/CMakeLists.txt` (new, `OBJECT` or `STATIC` libraries), `CMakePresets.json` (new: `windows-msvc-release`, `linux-release`, `linux-debug`), `cmake/version.h.in` or a compile definition for `APP_VERSION` and `UPDATE_REPO`; delete `lightscreen.pro`, `src.pri`, `version.pri`, `tests/**/*.pro`, `tests/common.pri`, `tools/**/*.pri`, `tools/UGlobalHotkey/uglobalhotkey.pro`; `.github/workflows/ci.yml`; `.gitignore`; `docs/build.md`; `README.md`.
- **What.** `cmake_minimum_required(VERSION 3.21)`, `project(... VERSION 2.5 LANGUAGES CXX)`, `find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Sql Concurrent Test)`, `qt_standard_project_setup()`, `qt_add_executable(lightscreen WIN32 ...)`, `qt_add_resources`, `.ui` files via `AUTOUIC`, Windows `.rc` via `RC_ICONS` equivalent (a small `lightscreen.rc`), the manifest via `/MANIFESTUAC:level='asInvoker'` link option, `target_link_libraries` for the Win32 and X11 libraries. Tests: `enable_testing()`, one `qt_add_executable` per test plus `add_test`, sources shared through an `OBJECT` library `lightscreen_core` containing everything but `main.cpp`. Delete the qmake files in the same PR so two build systems never drift.
- **Done-check.** CI green via `cmake --preset` + `cmake --build` + `ctest --output-on-failure` on both platforms; the Windows deploy artifact is produced by `windeployqt` as before; Linux smoke. Windows build by the owner with the preset.

### M14. Housekeeping: license headers, README, dead files

- **Tag:** mechanical. **Depends on:** M01 (for the badge). Can run at any point, in parallel with anything; rebase if it collides. **Merges:** thread.
- **Touches:** the 12 files without a license header (`dialogs/historydialog.*`, `tools/uploader/*`, `widgets/imguroptionswidget.*` unless M12 already deleted them, `widgets/pomfoptionswidget.*`), `dialogs/aboutdialog.ui` (delete; unused), the `OTHER_FILES += TODO.txt` line, `README.md` (fork notice, what changed, build pointer to `docs/build.md`, CI badge, no upstream badges), `docs/build.md`.
- **Done-check.** CI green. Every first-party `.cpp`/`.h` begins with the GPL v2 header, except `dialogs/areadialog.*`, which keeps its LGPL notice.

### M15. Release workflow

- **Tag:** mechanical. **Depends on:** M11. **Merges:** owner (touches releases).
- **Touches:** `.github/workflows/release.yml` (new), `docs/build.md`.
- **What.** On a tag matching `v*` (and on `workflow_dispatch` for a dry run): Windows build with the same Qt as CI, `windeployqt --release --no-translations`, zip as `<name>-<version>-win64.zip`, and create a *draft* GitHub Release with the zip attached. Unsigned, as decided. No Linux package: Linux users build from source (`docs/decisions.md`). The release notes are written by hand in Stage 7.
- **Done-check.** A `workflow_dispatch` run produces a draft release whose zip starts on the owner's machine (Windows QA). **Do not push a real tag before M16.**

### M16. Rename

- **Tag:** mechanical once the name is known. **Depends on:** every other step merged (it is last, so no later PR has to rename strings again), and the owner's choice of name. **Merges:** owner.
- **Touches:** `CMakeLists.txt` (project name, target, version, company and copyright strings, `UPDATE_REPO` if the GitHub repo is renamed), `main.cpp` (`setApplicationDisplayName(<Name>)`; `setOrganizationName("K")` and `setApplicationName("Lightscreen")` **stay**, see decisions), `lightscreenwindow.ui`, `lightscreenwindow.cpp`, `dialogs/*.ui` and `.cpp` (every `tr("... Lightscreen ...")`, window titles, the about text: fork notice, link to the fork, credit to the original author kept), `tools/os.cpp` (`setStartup`: write the new registry value / `.desktop` name and remove the old `Lightscreen` one so an upgrade does not leave two entries), `images/` (only if the owner supplies new icons; otherwise unchanged), `README.md`, `docs/`.
- **Done-check.** Tests pass, including a test asserting `QCoreApplication::organizationName() == "K"` and `applicationName() == "Lightscreen"` so nobody breaks the settings path by accident. CI green. **Windows QA:** upgrade over an existing 2.5 install keeps settings and history; window titles and about text show the new name; the startup entry is replaced, not duplicated.

## 4. Structural steps

The structural round starts after M11 (CMake) so that every structural PR builds with the final build system, and after Stage 5.

### S01. Take the dialogs out of `tools/screenshot`

- **Tag:** structural. **Depends on:** M11. **Merges:** owner.
- **Touches:** `tools/screenshot.{h,cpp}`, `tools/screenshotmanager.{h,cpp}`, `dialogs/areadialog.{h,cpp}`, `lightscreenwindow.cpp`, `tests/` (AreaDialog test adapts to the new constructor), `tests/check-includes.sh` (new) and `.github/workflows/ci.yml`.
- **Target.** `tools/` includes nothing from `dialogs/` or `widgets/`. `Screenshot::selectedArea()` no longer constructs `AreaDialog`; instead `Screenshot` grabs the desktop, emits `areaSelectionNeeded()` and waits (the same `mPixmapDelay` mechanism the window picker already uses). `ScreenshotManager` (the owner of screenshots and already the signal hub) runs `AreaDialog(const QPixmap &, const Screenshot::Options &)` and calls `screenshot->setArea(QRect)` or `screenshot->discard()`. `AreaDialog` reads the pixmap and options it was given instead of `mScreenshot->...`. Behaviour that must survive: magnifier (`Ctrl+M`), keyboard sizing, autoclose option, animations option, current-monitor geometry, `resultRect` scaling.
- **Done-check.** Include-graph check added to CI and green. Tests pass; the AreaDialog test is updated for the new constructor in the same PR with no change in expected values. Linux smoke. **Windows QA:** area capture with and without autoclose, magnifier toggle, `100x100` keyboard size, cancel with Escape.

### S02. Singletons out of `tools/`: `Screenshot` and the uploader talk through signals

- **Tag:** structural. **Depends on:** S01. **Merges:** owner.
- **Touches:** `tools/screenshot.{h,cpp}`, `tools/screenshotmanager.{h,cpp}`, `tools/uploader/uploader.{h,cpp}`, `tools/uploader/imageuploader.{h,cpp}`, `tools/uploader/pomfuploader.cpp`, `dialogs/historydialog.cpp`, `widgets/pomfoptionswidget.cpp`, `lightscreenwindow.cpp`, `updater/updater.cpp` (gets its network manager passed in), `tests/`.
- **Target.** `Screenshot` calls neither `ScreenshotManager::instance()` nor `Uploader::instance()`: it emits `saved(const QString &fileName)` and `uploadRequested(const QString &fileName, const QString &service)`, and `ScreenshotManager` connects them to `saveHistory` and to the uploader. `ImageUploader::loadSettings`/`saveSettings` take a `QSettings *` (the manager passes its own). `Uploader` is owned by `ScreenshotManager` (constructed with the settings pointer); `Uploader::instance()` may stay as a thin accessor on the manager for the dialogs, but nothing under `tools/uploader/` includes `tools/screenshotmanager.h`. After this step the include graph is a DAG: `app → dialogs/widgets → tools → tools/uploader`, `tools → os`.
- **Done-check.** Include-graph check green with the second rule (`tools/uploader/` includes only itself and Qt). Tests pass, plus a new flow test: a `Screenshot` with `upload = true` and the stub network manager ends with `Uploader::done` and one history row. CI green. Linux smoke. Windows QA: one upload.

### S03. User-configurable uploader replaces pomf

- **Tag:** structural. **Depends on:** S02, M12, M13. **Merges:** owner.
- **Touches:** `tools/uploader/pomfuploader.*` → `tools/uploader/customuploader.*` (rename and rewrite), `widgets/pomfoptionswidget.*` → `widgets/uploaderoptionswidget.*`, `dialogs/optionsdialog.{cpp,ui}`, `tools/uploader/imageuploader.cpp` (factory), `tools/uploader/uploader.cpp` (`serviceName` → `"custom"`), `tests/tst_uploader/`, `docs/`.
- **Spec** (from `docs/goals.md`). Settings under `upload/custom/`: `url` (full upload endpoint), `field` (multipart field name, default `files[]`), `headers` (one `Name: value` per line, optional), `linkPath` (dotted JSON path to the returned link, with integer indices for arrays, default `files.0.url`), `deletePath` (optional, same syntax). Request: `POST` multipart with the file under `field` and the extra headers. Response: HTTP 2xx and the path resolves to a non-empty string → `uploaded(file, link, deleteLink)`; otherwise `error` with the HTTP status or a parse message. One-time migration at first load: if `upload/pomf/pomf_url` is set and `upload/custom/url` is not, set `url = pomf_url + "/upload.php"`, `field = files[]`, `linkPath = files.0.url`; leave the old keys in place. The options widget has the five fields and a "Test upload" button that posts a 1×1 PNG and shows the resolved link or the error (replaces "Verify", which only worked for pomf). No host list.
- **Done-check.** Unit tests with the stub network manager: request has the right field name and headers; link path resolution incl. arrays and missing keys; HTTP 500, non-JSON, and empty-link cases emit `error`; the migration. CI green. **Manual QA** (any platform; the owner, against a host they trust): one real upload and the link opens.

### S04. Per-screen capture and DPI-correct area selection

- **Tag:** structural. **Depends on:** S01, M05. **Merges:** owner.
- **Touches:** `tools/screenshot.{h,cpp}` (`grabDesktop`), `tools/capturegeometry.{h,cpp}` (new: pure functions over a list of `{QRect logicalGeometry, qreal dpr}`), `dialogs/areadialog.{h,cpp}`, `tools/os.cpp` (`grabWindow` bounds via `DwmGetWindowAttribute(DWMWA_EXTENDED_FRAME_BOUNDS)` if Stage 5 bug 5 did not already land it), `tests/tst_capturegeometry/` (new).
- **Target.** `grabDesktop` grabs each `QScreen` separately (`screen->grabWindow(0)` gives that screen at its own device pixel ratio) and composes them onto one canvas whose size is the union of the screens' *device-pixel* rectangles; the cursor is drawn at device pixels using its own screen's ratio. `AreaDialog` covers the virtual geometry in logical coordinates, draws each screen's piece scaled to its logical rect, and `resultRect` maps the logical selection to device pixels per screen (not by one global ratio). The mapping functions live in `capturegeometry` so they are unit-testable with synthetic screen layouts: two screens at 100% and 150%, a screen at a negative offset, a screen above the primary.
- **Done-check.** Unit tests on the mapping with those layouts. CI green. **Windows QA on the owner's mixed-DPI setup** (the only real verification): full desktop capture matches the screens pixel for pixel; current-monitor capture on each monitor; area selection across the boundary of two monitors gives the pixels that were marked; cursor in the right place on each monitor; window capture of a window on the secondary monitor has no border strip. Closes bugs 4, 11, 12 and finishes 5.

### S05. PNG optimization queue (bug 10, second half)

- **Tag:** structural. **Depends on:** S02. **Merges:** owner.
- **Touches:** `tools/optimizer.{h,cpp}` (new), `tools/screenshotmanager.{h,cpp}`, `tools/screenshot.{h,cpp}` (`optimize()` moves out), `lightscreenwindow.cpp` (tray tooltip count), `tests/tst_optimizer/` with a stub `optipng` script.
- **Target.** One `Optimizer` owned by the manager: a FIFO of file paths, at most one `optipng` process at a time (two is acceptable if the owner prefers), `optipng -out <tmp>` then `QFile::rename` over the original on exit code 0 (so an interrupted run leaves the original intact; the Stage 5 fix may already have done the temp-file part inside `Screenshot::optimize`), the pending list persisted under the data directory and resumed at startup, `optimizationDone(fileName)` exactly once per file, and the tray tooltip/status showing "N optimizing" the way it shows uploads.
- **Done-check.** Tests: a burst of five files runs at most one process at a time; a stub that exits non-zero leaves the original byte-identical; a stub that never finishes is still pending after a manager restart and then runs; a missing binary emits `optimizationDone` once per file. CI green (Linux CI installs real `optipng` for one integration test). Windows QA optional.

### S06 (optional). One place for settings keys and defaults

- **Tag:** structural. **Depends on:** S02. **Merges:** owner.
- **Touches:** `tools/settings.{h,cpp}` (new), `lightscreenwindow.cpp`, `dialogs/optionsdialog.cpp`, `dialogs/previewdialog.cpp`, `dialogs/areadialog.cpp`, `dialogs/namingdialog.cpp`, `tools/screenshotmanager.cpp`.
- **Why.** The defaults are duplicated between `OptionsDialog::loadSettings` and `LightscreenWindow::screenshotAction`, and they disagree in places (`file/format` defaults to 1 in the dialog and to 0 by `toInt()` in the window; `options/cursor` to `true` in one and platform-dependent in the other before M03). A single `Settings` wrapper with named keys and one default per key makes the Stage 4 round-trip test the contract. Skip if the structural round is short on time; nothing else depends on it.
- **Done-check.** The settings round-trip test's expected values are unchanged (or every change is listed in the PR body as an intentional default fix). CI green.

### S07 (optional, owner's call). Jump list via `ICustomDestinationList`

- **Tag:** structural. **Depends on:** M07, M16 (the AppUserModelID should use the final name). **Merges:** owner. Windows QA only. Not scheduled; the owner asks for it if the jump list is missed.

## 5. Merge order and parallelism

| Order | Step | Can run in parallel with | Notes |
|---|---|---|---|
| 1 | M01 CI | M14 | before Stage 4 |
| 2 | M02 harness and seams | M14 | before Stage 4 |
| — | Stage 4 tests | each other (one session per module) | section 6 order |
| — | Stage 5 bugs | each other | section 7; bug 10's fix and bug 2's fix both touch `tools/screenshot.cpp`, rebase the later one |
| 3 | M03 Windows 10 minimum | M14 | |
| 4 | M12 remove Imgur | — | |
| 5 | M13 updater | — | |
| 6 | M04 API replacements | — | wide diff; nothing else in flight |
| 7 | M05 QDesktopWidget | — | |
| 8 | M06 sound | M08 | M06 and M08 both touch `tools/os.cpp`; small, rebase the later one |
| 9 | M07 WinExtras | M08 | |
| 10 | M08 X11Extras | M06, M07 | |
| 11 | M09 Qt 6 flip | — | owner's full QA pass |
| 12 | M10 purge Qt 5 branches | — | |
| 13 | M11 CMake | — | end of the mechanical round |
| 14 | M15 release workflow | S01 onward | no tag yet |
| 15 | S01 dialogs out of tools | M15 | start of the structural round |
| 16 | S02 singletons out of tools | — | |
| 17 | S03 custom uploader | S04, S05 | S04 and S05 both touch `tools/screenshot.cpp`: merge S04 first |
| 18 | S04 per-screen capture | S03 | |
| 19 | S05 optimizer queue | S03 | |
| 20 | S06 settings (optional) | — | |
| 21 | M16 rename | — | last; then tag `v3.0.0` (Stage 7) |

The coordinator may reorder within a round when a step is blocked on Windows QA, as long as the "depends on" lines hold.

## 6. Stage 4: test priorities

Tests record current behaviour, not intended behaviour. Each test file begins with `QStandardPaths::setTestModeEnabled(true)` and `QSettings::setDefaultFormat(QSettings::IniFormat)` so nothing touches the developer's real configuration, installs its own `ScreenshotManager` and `Uploader` through the M02 seams, and uses a `QTemporaryDir`. Widget tests run with `QT_QPA_PLATFORM=offscreen`; only a test that constructs `LightscreenWindow` needs Xvfb.

Seams that must exist before the tests can be written are all in M02: `ScreenshotManager(QSettings *, historyPath)` + `setInstance`; `Uploader::setInstance` + `setNetwork`; `Screenshot::extensionFor` + `uniqueName`.

In priority order, one Stage 4 session each:

1. **Settings round-trip** (`dialogs/optionsdialog`, `lightscreenwindow` option cache). Open `OptionsDialog` on empty settings, call `saveSettings()`, dump every key and value to a golden list checked into `tests/`; then set every widget to a non-default value, save, reload, compare. Also record what `LightscreenWindow::screenshotAction` reads (its defaults differ from the dialog's; the test documents both). This is the contract that M12, M13, M16, S03 and S06 must consciously update, and it is the first thing the Qt 6 port could silently break. Highest priority.
2. **Screenshot naming and saving** (`tools/screenshot`): `getName` for Numeric (with existing files, with `leadingZeros`, with `flip`), Date (fixed format), Timestamp (shape only), Empty; `extensionFor` for the four formats and an out-of-range value (records bug 2's current behaviour as *expected to change*, so the Stage 5 fix flips one assertion); `uniqueName` collision suffixes including its quirks (`cunt`/`count` loop, prefix matching); `save()` into a temporary directory for file-only, clipboard-only, both, `replace` on and off, each format with quality; `unloadPixmap` and the `.screenshot.` temp file; `confirm(false)` emits `finished` with `Cancel`. The code is kept and restructured by S01/S02, and M04 rewrites three of its calls.
3. **History** (`tools/screenshotmanager`): `saveHistory` with history on and off; `updateHistory` for a file that exists in the table and one that does not (this will expose bug 13 below; record whatever happens); `removeHistory`; `clearHistory`; portable mode detection with a `config.ini` next to a fake application dir if reachable through the seam, else skip; the history path. Kept code; S02 and M12 touch it.
4. **Uploader core** (`tools/uploader`): `serviceName`, `factory`, `ImageUploader::loadSettings`/`saveSettings` round trip, `Uploader::upload` → `done` with a stub reply, progress aggregation over two uploads, `cancel` emits `cancelAll` and clears the count, error path emits `error` then `done(file, "", "")`. For the pomf uploader itself: request field name and the `success`/`files[0].url` parsing, lightly, since S03 replaces it; these tests become S03's baseline.
5. **Hotkey widget** (`widgets/hotkeywidget`): `setHotkey`/`hotkey` round trip for `Print`, `Ctrl+Print`, `Ctrl+Alt+Print`, `Ctrl+PgUp`; `QTest::keyClick` with a modifier-only key leaves the hotkey unchanged; `Escape` gives "Invalid hotkey". M09 changes the key-combination line.
6. **Area dialog geometry** (`dialogs/areadialog`): `resultRect` with pixmap ratio 1 and 2; `keyboardResize` parsing (`100x100`, `0x10`, `abc`); `limitPointToRect`; handle positions after `updateHandles`. Kept; S01 changes its constructor, S04 its mapping.
7. **Main window plumbing** (`lightscreenwindow`, under Xvfb): `executeArguments` with each `--flag` reaches the expected slot (spy on `ScreenshotManager::take` via a subclass or on the mode in `Options`); `connectHotkeys` assigns ids 1 to 6 in the documented order (PR #37's id-0 bug); the "default argument means `--wake`" branch.
8. **`os` helpers** (`tools/os`): `getDocumentsPath` non-empty; `icon()` picks `/icons/` vs `/icons/inv/` by palette value; on Linux, `setStartup(true/false)` with `HOME` pointed at a temp dir writes and removes the `.desktop` file.

Light testing only (code marked for replacement or deletion): `updater/` (replaced in M13, which brings its own tests), `tools/uploader/imguruploader` and `widgets/imguroptionswidget` (deleted in M12: no tests), `widgets/pomfoptionswidget` (replaced in S03), `tools/windowpicker`, `dialogs/previewdialog`, `dialogs/historydialog`, `dialogs/namingdialog` (UI over tested logic; manual QA), `tools/SingleApplication` and `tools/UGlobalHotkey` (vendored; the Linux smoke covers them end to end).

Not testable headless at all: the capture paths (`grabDesktop`, `os::grabWindow`, the window picker's native lookups). They are covered by the Windows QA lists above.

## 7. Stage 5: bug triage

Rule: fix now when the code is kept through the structural round and the fix can be verified (a headless test, or a short Windows QA); defer when the step that rewrites that code carries the fix in its done-check.

| Bug | Decision | Where | Verification |
|---|---|---|---|
| 1 Window picker icon handle truncated (x64) | **Fix now** | Stage 5: `GetClassLongPtr(GCLP_HICON)`, then `GCLP_HICONSM`, then `SendMessage(WM_GETICON, ICON_BIG)` as fallbacks | Windows QA: hover a few windows in the picker |
| 2 `extension()` falls off the end | **Fix now** | Stage 5: validate `file/format` when read (fall back to PNG) and give `extensionFor` a default | Headless test (flip the Stage 4 assertion) |
| 3 Linux error sound file missing | Defer | M06 (sound rewrite; the option is hidden on Linux, so only the Windows path changes) | M06 done-check |
| 4 Mixed-DPI desktop capture | Defer | S04 | Owner's mixed-DPI QA |
| 5 Window capture margins on Windows 10 | **Fix now** (bounds only) | Stage 5: `DwmGetWindowAttribute(DWMWA_EXTENDED_FRAME_BOUNDS)` in `os::grabWindow`, fallback to `GetWindowRect` when it fails; the DPI interplay stays with S04 | Windows QA: capture a non-maximized window next to another; no stripe of the neighbour, no cut edge |
| 6 Update check fails silently | Defer | M13 (rewrite with tests) | M13 tests |
| 7 SSL errors ignored on "Windows 2003" | Defer | M03 (deleted) | M03 |
| 8 Dead macOS block | Defer | M03 (deleted) | M03 |
| 9 HDR browser brightness | **Defer, document** | Not in the plan. After M09, if the owner wants a spike: a Windows.Graphics.Capture path is a new capture back end (structural, Windows-only, needs the owner's HDR monitor for every check). Until then `docs/known-bugs.md` records it as known | — |
| 10 PNG optimization | **Split** | Stage 5 fixes the local defects in `Screenshot::optimize`: `optimizationDone` fires exactly once (early `return` after the missing-binary branch, `deleteLater` the process), pointer-to-member `finished` connect, and the in-place rewrite replaced by `optipng -out <tmp>` + rename. The queue, concurrency bound, persistence and tray count are S05 | Headless test with a stub `optipng` |
| 11 Current-monitor capture | Defer | M05 re-tests it; S04 owns the geometry | M05 Windows QA |
| 12 Area selection on HiDPI | Defer | S04 | S04 Windows QA |
| 13 `updateHistory` never updates (new, inferred) | **Fix now** | Stage 5 | Headless test |
| 14 History-dialog upload ignores the chosen service (new, confirmed) | Defer | M12 fixes the key (one line) | M12 settings test |

Two bugs found while reading the code for this plan, added to `docs/known-bugs.md` in the same PR:

- **13.** `ScreenshotManager::updateHistory` prepares `SELECT fileName FROM history WHERE URL IS NOT EMPTY AND fileName = ?`. `EMPTY` is not SQLite syntax, so `prepare` fails, `query.record().count()` is 0, and the function falls through to `saveHistory`, inserting a second row for the file instead of filling in its URL. Visible as duplicate rows in the history dialog after uploading an existing screenshot. Inferred from the code; the Stage 4 history test will show it.
- **14.** `HistoryDialog::upload` reads `options/upload/service` while every other site reads `upload/service`, and passes the raw value as the service name. Uploads started from the history dialog therefore always use the default service, whatever the options say.

Stage 5 sessions: bugs 1, 2, 5, 10 (local part), 13. Bug 1 and 5 can only be verified on the owner's machine; those sessions write the fix, get the Windows build from CI, and leave the PR for the owner to QA and merge.

## 8. Questions for the owner

Both answered on 2026-10-14; kept here as the record.

1. ~~Jump list~~ **Decided 2026-10-14: drop** in M07, with possible re-addition later. S07 stays as an optional step for that; it is not scheduled.
2. ~~Settings identity after the rename~~ **Decided 2026-10-14: keep.** The settings and history stay under organization `K`, application `Lightscreen`; upgrades are automatic and nothing is migrated.

## 9. Not in this plan, and why

- **Replacing the vendored libraries with maintained upstreams.** `SingleApplication` is 274 lines and `UGlobalHotkey` 816; both are ported in place (M04, M08, M09, M10) and listed in their `VENDORED.md`. Switching `SingleApplication` to `itay-grudev/SingleApplication` v3 changes `main.cpp`'s API use for no behaviour gain; it stays an option if the vendored copy ever misbehaves on Qt 6.
- **Splitting `LightscreenWindow` or `OptionsDialog`.** They are long but linear; the goals ask for boring code, not a redesign. S06 is the only concession.
- **Wayland, macOS, 32-bit, MinGW, an installer, translations.** Out of scope per `docs/goals.md`; the MinGW branch is removed in M03.
- **Linux packaging.** The release workflow ships the Windows zip only; Linux stays a build-from-source, CI-tested target.
