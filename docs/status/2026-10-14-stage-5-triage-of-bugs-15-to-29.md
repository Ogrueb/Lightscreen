# 2026-10-14: Stage 5 triage of bugs 15 to 29

- `docs/modernization-plan.md`, section 7: a row per bug 15 to 29 (decision, where, verification) and a session table for the new fix-now work. Reasoning: `docs/decisions.md`, Stage 5 triage entry. Each bug in `docs/known-bugs.md` now has a **Triage** line. Docs only; nothing was fixed.
- Deferred into plan steps, with their done-checks extended: 21 to M12 (and kept in S03), 22 to S03, 23 to S04. No change for 20.
- Fix-now Stage 5 sessions, highest priority first (all verified headlessly; each changes behaviour, so the owner merges):
  - **A, bug 27** (crash: upload error with the tray off). `LightscreenWindow::notify`. Can start now; shares `lightscreenwindow.cpp` with bug 2.
  - **B, bug 28** (runaway timer: second screenshot request during the delay). `screenshotAction`, `action`. Start after the bug 2 PR merges (same function).
  - **C, bugs 15 + 17** (data loss: collision naming overwrites a file). `Screenshot::uniqueName`. Can start now; shares `tools/screenshot.cpp` with bugs 2 and 10.
  - **D, bugs 16 + 18** (double `finished`, history row on failure, leftover temp file). `Screenshot::save`. Start after the bug 10 PR merges.
  - **E, bug 19** (WEBP saved as `-1`). Options dialog. Only if the bug 2 PR leaves it open.
  - **F, bug 24** (upper-case X in the typed area size). Can start now.
  - **G, bug 25** (Linux autostart directory missing). `os::setStartup`. Can start now; shares `tools/os.cpp` with bug 5.
  - **H, bug 26** (hotkey not settable from the keyboard). Can start now.
  - **I, bug 29** (`--quit` with no instance). `main.cpp` and a CI smoke step. Can start now.
