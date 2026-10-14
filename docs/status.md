# Status

What is done, what changed from the plan, and what is open. Newest first.

## 2026-10-14: M01 GitHub Actions CI

- `.github/workflows/ci.yml`: two jobs on every push and pull request. `linux` (ubuntu-latest, Qt 5.15 from apt) builds and runs the Linux smoke (start under Xvfb in portable mode, stop with `--quit`, expect exit 0 within ten seconds). `windows` (windows-latest, Qt 5.15.2 msvc2019_64, MSVC x64) builds, runs `windeployqt` and uploads `lightscreen-win64` as an artifact.
- README: the upstream AppVeyor and Coverity badges are replaced by the fork's CI badge (the rest of the README is M14's).
- `docs/build.md`: Linux section now verified, plus a "Continuous integration" section. Details the plan left open are in `docs/decisions.md` (M01 entry).
- Verified in the cloud container: the Linux build and the smoke script pass locally (Ubuntu 24.04, Qt 5.15.13). The Windows job is verified only by CI.
- Still open from the done-check: the optional one-time Windows QA (download the artifact and start it on the owner's machine).
- M02 can build on this: its `make check` steps go into the same two jobs.

## 2026-10-14: Stage 3 plan written

- `docs/modernization-plan.md`: 16 mechanical steps (M01-M16) and 7 structural steps (S01-S07, two optional), each one PR with dependencies, files, a done-check and a merge owner; an overall merge order; Stage 4 test priorities with the seams they need; Stage 5 bug triage. Decisions and rejected alternatives are in `docs/decisions.md` (Stage 3 entry).
- Two plan steps come **before Stage 4**: M01 (GitHub Actions CI on ubuntu and windows-latest) and M02 (test harness, three small seams). Stage 4 cannot start until both are merged.
- Stage 5 sessions: bugs 1, 2, 5 (bounds only), 10 (local defects only) and the new bug 13. Everything else is deferred to a named plan step.
- Two new bugs found while reading the code are in `docs/known-bugs.md`: 13 (`updateHistory` inserts instead of updating; inferred) and 14 (history-dialog upload ignores the chosen service; confirmed).
- Owner decisions taken in the Stage 3 thread (plan section 8): the Windows jump list is dropped in the port, with S07 as an optional later re-addition; the `K/Lightscreen` settings identity is kept at the rename. The fork's name is still needed before M16.
- PR #4 (`docs/upstream-prs.md`, bugs 11 and 12) was already merged when this stage started; the plan is written against `master` at `d92b3ca`.
- Nothing was built in this thread: the cloud container has no Qt installed. The plan's API claims come from the code and the Qt documentation. M01 is where a build first runs in CI.

## 2026-10-14: Upstream PRs recorded

- The upstream issue tracker (`ckaiser/Lightscreen`) is disabled, so there are no upstream issues to draw on. The 10 upstream PRs are summarized in `docs/upstream-prs.md`. The Linux `Success` clash (#40) and the active-monitor capture bug (#38) are already fixed in the code. Multi-monitor and HiDPI capture (#38, #35) are worth re-testing.
- Folded the upstream PR findings into `docs/known-bugs.md` (bugs 11 and 12, plus a note on bug 4). #40 and #44 need no entry, since the Linux build already works.

## 2026-10-14: goals.md and known-bugs.md drafted

- `docs/goals.md`: purpose, the six-point definition of "resilient", platform and compatibility commitments, out-of-scope list. Decisions taken by the owner in the drafting thread: Qt 6, Windows 10 minimum, Linux X11 kept as a CI-tested target, update check moved to GitHub Releases. Recorded in `docs/decisions.md`. Only the fork's name is still open, and it is not needed before Stage 3.
- `docs/known-bugs.md`: ten entries. Three confirmed from the code (window-picker icon handle, `extension()` missing return, missing Linux error sound), five inferred from the code and awaiting reproduction (mixed-DPI desktop capture, window capture margins, silent update failure, dead SSL workaround, dead macOS block), two reported by the owner (HDR browser brightness, PNG optimization queue).
- Upstream's issue tracker is disabled, so there was no issue list to mine. `docs/upstream-prs.md` (being written by another thread) may add entries from the five unmerged upstream PRs; fold those in when it lands.
- Stage 3 can start once this PR is merged.

## 2026-10-14: Stage 2 inventory

- `docs/inventory.md`: dependencies, module map, deprecated and Qt 6-removed APIs, test coverage, build/CI/packaging. Survey only; no code changed.
- Correction to `docs/build.md`: `Screenshot::extension()` (C4715) does handle WEBP inside its `switch`. The missing return is reached only when the format read from settings is outside 0-3. See `docs/inventory.md`, "Other compiler-flagged code".
- Not checked from the Stage 2 session (outbound access limited), marked *(not verified here)* in the inventory: whether `lightscreen.com.ar` is still up (the updater and the pomf host list use it), and the end date of Qt 5.15 support.
- Still open: CI for Windows and Linux. (`docs/goals.md` and `docs/known-bugs.md`: see the entry above.)

## 2026-10-14: Submodules vendored

- SingleApplication and UGlobalHotkey are now plain files in the repo instead of submodules (see `docs/decisions.md`). A plain `git clone` is enough to build; the `git submodule update --init` step is gone from `docs/build.md`.
- Linux build check, done in the cloud container (Ubuntu 24.04, Qt 5.15.13 from apt): `qmake` and `make` succeed with the vendored sources, and the binary starts and keeps running under Xvfb. Two findings for whoever owns the Linux build:
  - `qtbase5-private-dev` is needed in addition to the packages listed in `docs/build.md`, because UGlobalHotkey includes `qpa/qplatformnativeinterface.h`.
  - With `QT_QPA_PLATFORM=offscreen` (no X server) the app segfaults at startup; it needs a real or virtual X display.
- Windows build verified by the owner on a fresh clone (VS 2022, Qt 5.15.2): builds, launches, same 14 warnings as Stage 1. Existing checkouts should run `git submodule deinit --all` (or reclone) after pulling this change.

## 2026-10-14: Stage 1 done

- Windows x64 release build works on VS 2022 + Qt 5.15.2 with no source changes; see `docs/build.md`.
- Open from Stage 1: Linux build verification, CI for Windows and Linux, `docs/goals.md` and `docs/known-bugs.md` (by the owner).
