# Status

## Done

- **Stage 1: Fork and get it building** (PR #1). Builds on Windows with VS 2022 and Qt 5.15.2, no code changes. Details in `docs/build.md`.
- **Stage 2: Inventory.** `docs/inventory.md`: dependencies, module map, deprecated APIs, test coverage, build/CI/packaging. Survey only; nothing was built and no code changed.

## Changes from the plan

- `docs/status.md` started in Stage 2 rather than Stage 4, so the CLAUDE.md "update status on exit" rule has a file to update.

## Corrections to earlier notes

- `docs/build.md` says `Screenshot::extension()` (C4715) has no return for formats outside its `switch` because WEBP is still in the enum. WEBP is handled inside the `switch`; the missing return is reached only when the format read from settings is outside 0-3. See `docs/inventory.md`, "Other compiler-flagged code".

## Open items

- Linux build not verified; no CI (Stage 1 leftovers).
- Submodules still point at the original author's repos (vendoring in progress in a separate thread).
- `docs/goals.md` and `docs/known-bugs.md` (owner), needed before Stage 3.
- `docs/decisions.md` does not exist yet.
- Version and status data that could not be checked from the Stage 2 session (outbound access limited): `lightscreen.com.ar` availability (used by the updater and the pomf host list), and the end of Qt 5.15 support. Marked *(not verified here)* in the inventory.
