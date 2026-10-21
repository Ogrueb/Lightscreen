# Goals

What this fork is for, what "resilient" means here, what it commits to, and what is out of scope. Stage 3 (the modernization plan) treats this file as its brief. Everything here is decided by the owner unless marked **[open]**.

Draft status: second draft, 2026-10-14, after the owner's answers on Qt, Windows, Linux and the update check. Not yet merged.

## Purpose

Keep Lightscreen usable as a small, fast, tray-resident screenshot tool on a current operating system and toolchain, for as long as practical, with as little ongoing effort as practical. This is a maintenance fork, not a redesign: the feature set stays what it is unless a feature can no longer work.

## What "resilient" means

1. **Builds from a plain clone on a current toolchain.** No submodules, no dead download hosts, no author-specific build steps. (Already true as of Stage 1 and PR #2.)
2. **No dependency on anything one person runs.** No calls to `lightscreen.com.ar`, no hardcoded API credentials, no update server. Online services are either public infrastructure (GitHub Releases) or the user's own choice (a configurable uploader).
3. **Builds and tests in CI on every push**, on Windows and on each other platform still supported, so a broken build is noticed by a machine and not by a user.
4. **Only maintained dependencies.** Qt at a version that still receives open-source releases; vendored libraries that are either tiny or replaced by a maintained upstream.
5. **Behavior under test.** Safety-net tests (Stage 4) record current behavior of the logic that can be tested headless, so later restructuring does not change what the app does silently.
6. **Boring code.** Removed and deprecated APIs gone; the module cycles in `docs/inventory.md` broken enough that a module can be replaced on its own; no Windows XP era code paths.

Not part of "resilient": new features, a new UI, a plugin system, a rewrite in another language or framework.

## Platform and toolchain commitments

- **Qt 6** (decided 2026-10-14). Qt 5.15 has no further open-source releases, so a fork on it would rot on a fixed date; Qt 6 is where fixes, binaries and tooling are. The port is one plan step: the `winextras` and `x11extras` modules are gone, so the taskbar progress button, the jump list and the DWM blur are reimplemented with Win32 calls or dropped (the plan decides per feature), the X11 window-picker code moves to xcb, and about 20 smaller API replacements land across most files (`docs/inventory.md` section 4). Target the current Qt 6 LTS at the time of the port.
- **Windows 10 or later** (decided). Qt 6 supports nothing older. The XP linker flags and the `WV_XP` / `WV_2003` runtime checks are removed; Windows 7, 8 and 8.1 are not tested and not promised.
- **Linux, X11, kept as a tested target** (decided). It is the only target cloud threads and CI can build and run end to end, so it is where the safety-net tests and most bug fixes are exercised. The owner does not test Linux by hand; "tested" means CI and cloud threads. Wayland is out of scope: global hotkeys and window picking have no portable Wayland path.
- **macOS:** not a target. The code has no macOS build branch and never did.
- **Architecture:** 64-bit only. The 32-bit Windows build is dropped.
- **Build system:** qmake stays until a plan step replaces it; CMake is the expected end state because Qt 6 and CI tooling are built around it. The switch is one plan step, not a prerequisite.
- **C++ standard:** C++17 (the Qt 6 minimum).

## Compatibility commitments

- **Settings and history carry over.** A user upgrading from upstream 2.5 keeps their settings and screenshot history. Note that the settings location is keyed on organization name `"K"` and application name `"Lightscreen"` (`main.cpp:38-39`); renaming the app must either keep those keys or migrate them once.
- **Portable mode stays.** `config.ini` next to the executable keeps working as it does now.
- **Hotkeys, naming patterns, file formats and the capture modes** (desktop, current monitor, area, window, window picker) keep their current behavior unless a known bug says otherwise.
- **Uploads:** Imgur is removed (decided). The pomf uploader becomes a user-configurable uploader: upload URL, form field, optional headers, and the JSON path to the returned link. Existing pomf settings are migrated or re-entered; a pre-filled host list is not promised.
- **Update check:** moves to the fork's GitHub Releases (decided, "for now"): the app asks `api.github.com/repos/<owner>/<fork>/releases/latest` and compares `tag_name` to its version. A failed request must say so, not report "up to date" (see `docs/known-bugs.md`, bug 6). Removing the check altogether stays an option if it proves a nuisance.

## Identity

- **Name: [open]** The fork ships under a new name so bug reports stay off the original author's plate (decided). The name is not chosen yet; until it is, the code keeps "Lightscreen" and the release process waits.
- Company and copyright strings in `CMakeLists.txt`, the organization name in `main.cpp`, icons and the about text change together in one plan step once the name is chosen.
- **License:** GPL v2, unchanged. The LGPL-based `areadialog` keeps its notice. Files with no license header get one.
- **Releases:** unsigned (decided). Published as GitHub Releases with a zipped `windeployqt` output; an installer is a later nice-to-have.

## Out of scope

Decided and not to be reopened in this fork:

- Contact with, or contributions back to, the original author or repository.
- Any call to `lightscreen.com.ar`.
- Imgur, or any uploader with credentials baked into the binary.
- Code signing.
- Windows XP, Vista, 7, 8 and 8.1; 32-bit builds.
- macOS and Wayland.
- New capture features (video, scrolling capture, annotation, OCR) and any UI redesign.
- Translations beyond what exists (none are shipped; `windeployqt --no-translations` stays).

## Open questions, in one place

| # | Question | Options | Recommendation |
|---|---|---|---|
| 1 | Fork name | — | needed before the rename step, not before Stage 3 |

Settled 2026-10-14: Qt 6; Windows 10 minimum; Linux kept as a CI-tested target; update check via GitHub Releases.
