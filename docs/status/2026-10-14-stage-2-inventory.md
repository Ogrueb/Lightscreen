# 2026-10-14: Stage 2 inventory

- `docs/inventory.md`: dependencies, module map, deprecated and Qt 6-removed APIs, test coverage, build/CI/packaging. Survey only; no code changed.
- Correction to `docs/build.md`: `Screenshot::extension()` (C4715) does handle WEBP inside its `switch`. The missing return is reached only when the format read from settings is outside 0-3. See `docs/inventory.md`, "Other compiler-flagged code".
- Not checked from the Stage 2 session (outbound access limited), marked *(not verified here)* in the inventory: whether `lightscreen.com.ar` is still up (the updater and the pomf host list use it), and the end date of Qt 5.15 support.
- Still open: CI for Windows and Linux. (`docs/goals.md` and `docs/known-bugs.md`: see the entry above.)
