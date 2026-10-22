# 2026-10-22: Remaining work after S03

Snapshot of what is left once M03-M15 and S01-S03 are merged (S03 = PR #50). No PRs were open at the time of writing. Nothing in the code changed; this entry only collects the open items from the plan, the bug files and the Windows QA list.

## Plan steps still to do

| Step | Status | Notes |
|---|---|---|
| S04 per-screen capture and DPI-correct area selection | Not started | Carries bugs 4, 12 and 23 (and the DPI part of bug 5). Real verification only on the owner's mixed-DPI setup. Merge before S05 (both touch `tools/screenshot.cpp`). |
| S05 PNG optimization queue | Not started | Second half of bug 10, plus the optipng/oxipng choice (`docs/decisions/2026-10-21-optipng-and-oxipng-both-selectable.md`): key `options/optimizer`, fallback to the other tool, tests under both names. |
| S06 settings keys and defaults (optional) | Not started | Owner's call whether to run it; nothing depends on it. |
| M16 rename | Blocked | Needs the fork name from the owner. Runs last. Stage 7 (review, release notes, tag `v3.0.0`) follows. |
| S07 jump list (optional) | Not scheduled | Only if the owner asks for it; depends on M16. |

## Known bugs still open

- **4, 12, 23** (mixed-DPI desktop capture, HiDPI area selection, fractional-scaling pixel loss): S04.
- **9** (HDR browser windows captured brighter): documented only; a Windows.Graphics.Capture spike only if the owner wants it.
- **11** (current-monitor capture): needs the by-hand re-test in `docs/windows-qa/2026-10-15-qscreen-capture-and-placement.md`.
- **1, 5**: fixed, waiting on their Windows QA.
- **16**, leftover: an upload-only screenshot (file off, no clipboard) still emits `finished` early and again when the upload succeeds.
- **20** (PNG quality forced to 80): deliberate no-change; revisit only if S05 makes PNG compression configurable.

Smaller leftovers from earlier entries: the deprecated `QMessageBox::question` overload in `lightscreenwindow.cpp` (M10 status); dialogs and the window still reach the manager through `ScreenshotManager::instance()` (S02 status).

## Windows QA pending

16 checks in `docs/windows-qa/`, none run yet: window capture bounds, window picker icons, go to folder and naming filter, hotkey from keyboard, Imgur removed, QScreen capture and placement, sound cues, update check, WEBP format, taskbar without WinExtras, the Qt 6 full pass, area capture after S01, CMake build, custom uploader, release dry run, upload after S02. Suggested timing: one pass after S04 merges, since S04 adds the mixed-DPI checks.

## Decisions needed from the owner

1. The fork name (blocks M16).
2. Whether to run S06.
3. Whether to try a capture spike for bug 9 or leave it documented.
