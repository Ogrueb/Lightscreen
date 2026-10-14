# Decisions

Decisions, their reasons and the alternatives rejected. Newest first.

## 2026-10-14 (Stage 5, bug 1): `WM_GETICON` is sent with a timeout

- **The window picker's last icon fallback uses `SendMessageTimeout(..., SMTO_ABORTIFHUNG, 100 ms)`,** not the plain `SendMessage` the triage table names. The picker asks on every mouse move, and a plain `SendMessage` to a hung window would freeze the picker until that window answers. A window that times out simply shows no icon. *Rejected:* plain `SendMessage` (can block indefinitely).

## 2026-10-14 (Stage 5, bug 13): what `updateHistory` does with a row that already has a URL

- **The newer upload overwrites the URL and delete URL** of every row for that file name, and refreshes the time. Re-uploading means the user wants the new link; the old delete link is lost from history, as it was for files never recorded. *Rejected:* keeping the first URL (the history would show a link the user replaced); a new row per upload (that is the bug).
- **An empty delete hash stores an empty delete URL,** matching `saveHistory`, instead of the bare `https://imgur.com/delete/` the update branch used to write. *Rejected:* keeping the bare prefix (a dead link in the history dialog for uploaders without a delete hash, such as pomf).
## 2026-10-14 (Stage 5, bug 2): where an out-of-range format is caught

- **Checked twice, one rule:** `Screenshot::formatFromSetting` is used when the window reads `file/format`, and the `Screenshot` constructor passes `mOptions.format` through it too, so any caller that builds `Options` by hand also gets PNG. `extensionFor` keeps its own `.png` default for direct callers. *Rejected:* fixing only `extensionFor` (the file would be named `.png` while `Options::format` still holds the bad value, and the PNG-only paths, quality 80 and optimization, would be skipped).

## 2026-10-14 (Stage 4, main window): observing `LightscreenWindow` without seams

- **Screenshot modes are read from `ScreenshotManager::windowCleanup`,** which carries the `Screenshot::Options` of every finished screenshot. The test lets the real capture run under Xvfb with file and clipboard off, and a 20 ms timer rejects the area selector and closes the window picker, so those end as cancelled screenshots. *Rejected:* a `ScreenshotManager` subclass (`take` is not virtual) or reading the private `mLastMode` (would need `#define private public`).
- **Hotkey ids are read by feeding `UGlobalHotkeys::nativeEventFilter` a synthetic `xcb_key_press_event_t`** for the registered key, after detaching the window's handler from `activated`; a second test emits `activated(id)` into the window. *Rejected:* real key presses through XTest (an extra library, and the X server would deliver them to the grab asynchronously).
- **`--folder` is observed with `QDesktopServices::setUrlHandler("file", ...)`,** so no file manager starts. **`--quit` runs inside `QApplication::exec()`** in the last test function, because `quit()` outside a running loop would leave Qt's quit flag set for every later dialog.
- **The whole test skips on Windows,** for the same reason as the settings test's window tests.

## 2026-10-14 (Stage 4, settings): how the settings round-trip test records the contract

- **Golden lists are plain `key=value` files next to the test** (`tests/tst_settings/defaults.golden`, `roundtrip.golden`), read back from the INI file the dialog wrote, with machine paths replaced by `<documents>`, `<tmp>`, `<cwd>`. Platform differences are `linux:`/`windows:` lines that replace the common line. `LIGHTSCREEN_PRINT_GOLDEN=1` prints the actual list, so a step that changes a default (M12, M13, M16, S03, S06) regenerates and reviews the diff. *Rejected:* expected values inline in C++ (harder to diff in review); one golden file per platform (the two would drift).
- **The `LightscreenWindow` tests are skipped on Windows,** not run offscreen. The window's `applySettings` calls `os::setStartup`, which on Windows writes the real `HKCU\...\Run` key and would remove the owner's autostart entry when the tests run on their checkout. On Linux the test points `HOME` at a temporary directory instead, which covers the equivalent `~/.config/autostart` file. *Rejected:* a seam for `os::setStartup` (a production change in a tests-only session).
- **What `screenshotAction` reads is observed without a seam:** the test calls it with mode 5 (not a `Screenshot::Mode` enumerator), so `Screenshot::take()` captures nothing and cancels, and `ScreenshotManager::windowCleanup` still carries the options. *Rejected:* a real capture under Xvfb (writes files and the clipboard, slower, and depends on the X server).

## 2026-10-14 (Stage 4, screenshot naming): bug 2 is recorded as a skipped assertion

- **`extensionFor` with an out-of-range format is not called.** The plan asks to record its current behaviour, but today it falls off the end of a non-void function (undefined behaviour), so there is no behaviour to record and calling it could crash the test run. `tst_screenshot::extensionFor_outOfRange` holds the intended assertion (`.png`) behind a `QSKIP`; the Stage 5 fix deletes the `QSKIP` line. *Rejected:* calling it and asserting whatever GCC and MSVC happen to return (undefined, and could differ between runs or compilers).

## 2026-10-14 (M02): details of the test harness the plan left open

- **`version.pri` is included from `src.pri`**, not `lightscreen.pro`, so the tests compile with exactly the app's defines (it defines `APP_VERSION`). *Rejected:* keeping it in `lightscreen.pro` (tests and app would build with different defines).
- **The MSVC branch is split by kind:** compile flags and libraries (`_ATL_XP_TARGETING`, `_USING_V110_SDK71`, the Win32 `LIBS`) go to `src.pri`; the linker subsystem and manifest flags and `embed_manifest_exe` stay in `lightscreen.pro`, since they describe the executable, not the code. M03 removes the XP flags from both places. *Rejected:* moving the whole block (the test executables would get the app's icon, manifest and subsystem settings).
- **The seams do not take ownership.** `setInstance` and `setNetwork` replace a pointer and delete nothing; the test owns what it installs and resets the singleton to `nullptr` afterwards. *Rejected:* deleting the previous instance (the app's own singleton has no owner to notify, and a test-owned stack object must not be deleted).
- **The original `ScreenshotManager` constructor delegates to the new one, then sets the settings, history path and portable flag as before.** This keeps its body unchanged rather than computing the arguments in the initializer list. *Rejected:* computing them in the delegation call (three `config.ini` checks or a helper, for no gain).
- **Out-of-tree builds:** the tests build in `build-tests/`, git-ignored next to `build/`.

## 2026-10-14 (M01): details of the CI workflow

The plan left these open; each is the simplest option that fits the decisions above.

- **The smoke config also sets `options/disableUpdater=true`.** The app schedules an update check five seconds after start, against `lightscreen.com.ar`. The smoke quits after two seconds so it would not fire, but the setting makes the "no calls to lightscreen.com.ar" rule hold by construction rather than by timing. *Rejected:* relying on the timing.
- **The smoke script lives inline in `ci.yml`,** written to `build/smoke.sh` at run time. The plan names only the workflow file, and the script is 30 lines. *Rejected:* a separate `scripts/` file (a new top-level directory for one script; revisit if M02 or later steps want to call it).
- **Qt on Windows comes from `jurplel/install-qt-action@v4`** with its built-in cache, MSVC from `ilammy/msvc-dev-cmd@v1`. Actions are pinned by major tag, as the plan says. *Rejected:* calling `aqtinstall` directly (more steps, and caching would be by hand).
- **The Windows artifact is the deploy folder as uploaded by `actions/upload-artifact`,** which zips it. *Rejected:* zipping it first (the download would be a zip inside a zip).
- **Push runs are limited to `master`;** branches get CI through their pull request. Running on every push ran each PR twice. The workflow also runs on `workflow_dispatch` and cancels superseded runs on the same ref.
- **`actions/checkout` and `actions/upload-artifact` are on v5** (Node 24), since v4 triggers GitHub's Node 20 deprecation warning. `ilammy/msvc-dev-cmd@v1` still warns; it has no newer major.

## 2026-10-14 (Stage 3): shape of the modernization plan

The plan is `docs/modernization-plan.md`. The decisions it rests on, each with what was rejected:

**Port on Qt 5.15 first, flip last.** Every API that Qt 6 removed and Qt 5.15 can already replace is swapped while still building on 5.15 (M04, M05), the winextras and x11extras uses go behind two small shims (M07, M08), and only then does CI switch to Qt 6 (M09). The code builds and the safety-net tests pass after every one of those PRs. *Rejected:* a single Qt 6 branch that is red until everything is ported (unreviewable, and nothing to merge for weeks); porting file by file on Qt 6 (same problem).

**CI first, tests second, both before Stage 4.** M01 and M02 are plan steps that run before the safety-net tests exist, because the tests need a harness, seams and a Windows compile to be useful. *Rejected:* writing tests without seams (would force each test to run the real singletons against the developer's settings and network).

**Tests run on both CI platforms.** Linux under Xvfb, Windows with the offscreen platform. This gives the Stage 4 tests Windows execution without the owner's machine. *Rejected:* Linux-only tests (the Windows build would stay unverified beyond "it compiles").

**Seams are constructor injection and `setInstance`, nothing more.** `ScreenshotManager` and `Uploader` get a `setInstance` and a way to pass a settings object, a history path and a network manager. *Rejected:* a dependency-injection layer or interfaces for everything (not boring code).

**CI pins the same Qt on both runners through aqtinstall.** The current Qt 6 LTS at the time of M09, installed by `aqtinstall` on ubuntu and windows-latest, so one Qt version is the reference. Ubuntu's packaged Qt 6 is used only for local builds in cloud containers. *Rejected:* apt's Qt 6 in CI (older than the LTS, and a different version from Windows).

**QtMultimedia goes; sound is Win32 `PlaySound`.** The only use is `QSound::play`, which Qt 6 removed. The options dialog already hides the sound option on Linux, so the only platform with sound is Windows, where `PlaySound` from memory does the job without any Qt module. *Rejected:* `QSoundEffect` (pulls Qt Multimedia and, from 6.5, its ffmpeg backend into the deploy for one wav); writing a wav player on `QAudioSink` (new code for no gain); adding Linux sound (the option is hidden there today and the goals say no new features).

**Taskbar progress and overlay icon are kept; DWM glass and the jump list are dropped.** `ITaskbarList3` is a small, stable COM interface and the progress/overlay are visible every upload. `enableBlurBehindWindow` produces a transparent rather than blurred window on Windows 10, so dropping it is a visual fix. The jump list is about 150 lines of `ICustomDestinationList` that only the owner can test, for actions the tray menu and hotkeys already provide. Owner decision, 2026-10-14: drop, with possible re-addition later; S07 stays in the plan as an optional step for that. *Rejected:* reimplementing all three (cost, and only the owner can QA them); dropping the taskbar progress too (visible regression for every upload).

**The vendored libraries are ported in place.** `SingleApplication` (274 lines) needs one `QRegExp` line changed; `UGlobalHotkey` (816 lines) needs the xcb connection from `QNativeInterface` and a `qintptr` signature. Each change is recorded in the directory's `VENDORED.md`. *Rejected:* `itay-grudev/SingleApplication` v3 (maintained and Qt 6 ready, but a different API in `main.cpp` and a second build integration, for a library this small); dropping `UGlobalHotkey` for a new hotkey layer (no maintained drop-in exists for Win32 + xcb).

**qmake until the port is done, then CMake in one PR, qmake deleted in the same PR.** Qt 6 still ships qmake, so the port does not need CMake, and porting while also changing build systems doubles the search space when something fails. CMake comes right after (M11) because Qt 6, CTest and the CI tooling are built around it. *Rejected:* CMake before the port; keeping both build files (they would drift).

**Imgur and the dead host go before the port.** M12 deletes the Imgur code and M13 rewrites the updater right after M03, so M04 to M09 never port code that is about to be deleted. *Rejected:* porting everything and deleting afterwards.

**The updater rewrite is tagged mechanical.** It replaces 110 lines with a fully specified request, parse and compare, has a headless test, and depends on nothing structural. The repository it checks is a build-time define so the rename step changes it in one place. *Rejected:* removing the update check (kept as a fallback in the goals, not chosen).

**Release artifact: a Windows zip only, from a tag-triggered workflow, as a draft release.** Linux users build from source; the goals promise a zipped `windeployqt` output and nothing else. *Rejected:* AppImage or a Linux tarball (packaging work for a target nobody tests by hand); an installer (listed as a later nice-to-have).

**The rename keeps the settings identity** (owner decision, 2026-10-14: "keep for now"). `setOrganizationName("K")` and `setApplicationName("Lightscreen")` stay so settings, history and the single-instance key are untouched by the rename; only the display name, executable, strings, icons and the autostart entry change. *Rejected:* migrating to a new settings location (more code and QA for a cosmetic folder name). Can be revisited after the rename if the folder name bothers anyone.

**The MinGW branch is removed.** It is unmaintained (`libgdi32`-style names, overwrites `QMAKE_CXXFLAGS`), was never tried in this fork, and CI builds with MSVC. *Rejected:* fixing it (no one would test it).

**Bug triage rule.** Fix now when the code is kept through the structural round and the fix can be verified; defer when a plan step rewrites that code and carries the fix in its done-check. Bug 10 is split: its local defects are fixed in Stage 5, the queue is S05. Bug 9 (HDR) is documented, not planned, because a fix means a new capture back end that only the owner's monitor can verify.

**Structural round scope.** Two cycle-breaking steps (S01, S02), then the three behaviour steps the goals ask for (custom uploader S03, per-screen capture S04, optimizer queue S05). S06 (settings in one place) is optional. *Rejected:* splitting `LightscreenWindow` and `OptionsDialog` (long but linear; the goals ask for boring code, not a redesign).

## 2026-10-14: Qt 6, Windows 10 minimum, Linux kept, update check via GitHub Releases

**Decision.** The fork moves to Qt 6 (current LTS at the time of the port). Minimum Windows is 10; 32-bit builds, XP/Vista/7/8 code paths and the XP linker flags go. Linux X11 stays a target, tested by CI and cloud threads rather than by the owner. The update check is pointed at the fork's GitHub Releases instead of `lightscreen.com.ar`. Full statement in `docs/goals.md`.

**Why.** Qt 5.15 has no further open-source releases, so a fork on it would be "resilient" only until the next OS or compiler change broke Qt 5; the port cost (winextras/x11extras replacements plus about 20 small API changes) is one mechanical plan step. Qt 6 forces Windows 10 and C++17, both acceptable to the owner. Linux is the only target CI and cloud threads can run end to end, so keeping it gives the safety-net tests somewhere to run. The GitHub Releases check keeps the existing user experience with no server to run.

**Rejected.**
- Staying on Qt 5.15: zero porting cost now, dead platform later.
- Windows 7 minimum: not supported by Qt 6.
- Dropping Linux or keeping it build-only: loses the only end-to-end test platform the project has.
- Removing the update check: kept as a fallback if the GitHub check proves a nuisance ("for now" was the owner's wording).

## 2026-10-14: Vendor SingleApplication and UGlobalHotkey instead of using submodules

**Decision.** The two git submodules were replaced by plain copies of their source in the repository, at the same paths (`tools/SingleApplication`, `tools/UGlobalHotkey`) and at the commits that were pinned (`ckaiser/SingleApplication@c6378ee`, `ckaiser/UGlobalHotkey@231b101`). The files are byte-identical to those commits. `.gitmodules` is gone. Each directory has a `VENDORED.md` recording its origin, commit and license (MIT for SingleApplication, public domain for UGlobalHotkey).

**Why.** Both submodules pointed at the original author's own forks, which are as unmaintained as the app. Every clone of the fork depended on those dormant repos staying online. The libraries are small (about 1,100 lines together) and already diverge from any maintained upstream, so there is nothing to gain from tracking them as separate repos. Keeping the same paths meant no build file or `#include` had to change.

**Rejected.**
- Forking both repos under the fork's account: keeps their history, but adds two repos to maintain for little code.
- Switching SingleApplication to the maintained `itay-grudev/SingleApplication` now: `main.cpp` uses this copy's custom API (`instanceArguments`), so it is a code change. Revisit with the Qt 6 decision.
