# Decisions

Decisions, their reasons and the alternatives rejected. Newest first.

## 2026-10-14 (Stage 3): shape of the modernization plan

The plan is `docs/modernization-plan.md`. The decisions it rests on, each with what was rejected:

**Port on Qt 5.15 first, flip last.** Every API that Qt 6 removed and Qt 5.15 can already replace is swapped while still building on 5.15 (M04, M05), the winextras and x11extras uses go behind two small shims (M07, M08), and only then does CI switch to Qt 6 (M09). The code builds and the safety-net tests pass after every one of those PRs. *Rejected:* a single Qt 6 branch that is red until everything is ported (unreviewable, and nothing to merge for weeks); porting file by file on Qt 6 (same problem).

**CI first, tests second, both before Stage 4.** M01 and M02 are plan steps that run before the safety-net tests exist, because the tests need a harness, seams and a Windows compile to be useful. *Rejected:* writing tests without seams (would force each test to run the real singletons against the developer's settings and network).

**Tests run on both CI platforms.** Linux under Xvfb, Windows with the offscreen platform. This gives the Stage 4 tests Windows execution without the owner's machine. *Rejected:* Linux-only tests (the Windows build would stay unverified beyond "it compiles").

**Seams are constructor injection and `setInstance`, nothing more.** `ScreenshotManager` and `Uploader` get a `setInstance` and a way to pass a settings object, a history path and a network manager. *Rejected:* a dependency-injection layer or interfaces for everything (not boring code).

**CI pins the same Qt on both runners through aqtinstall.** The current Qt 6 LTS at the time of M09, installed by `aqtinstall` on ubuntu and windows-latest, so one Qt version is the reference. Ubuntu's packaged Qt 6 is used only for local builds in cloud containers. *Rejected:* apt's Qt 6 in CI (older than the LTS, and a different version from Windows).

**QtMultimedia goes; sound is Win32 `PlaySound`.** The only use is `QSound::play`, which Qt 6 removed. The options dialog already hides the sound option on Linux, so the only platform with sound is Windows, where `PlaySound` from memory does the job without any Qt module. *Rejected:* `QSoundEffect` (pulls Qt Multimedia and, from 6.5, its ffmpeg backend into the deploy for one wav); writing a wav player on `QAudioSink` (new code for no gain); adding Linux sound (the option is hidden there today and the goals say no new features).

**Taskbar progress and overlay icon are kept; DWM glass and the jump list are dropped.** `ITaskbarList3` is a small, stable COM interface and the progress/overlay are visible every upload. `enableBlurBehindWindow` produces a transparent rather than blurred window on Windows 10, so dropping it is a visual fix. The jump list is about 150 lines of `ICustomDestinationList` that only the owner can test, for actions the tray menu and hotkeys already provide. The jump list is open question 1 in the plan; S07 reinstates it if the owner says so. *Rejected:* reimplementing all three (cost, and only the owner can QA them); dropping the taskbar progress too (visible regression for every upload).

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
