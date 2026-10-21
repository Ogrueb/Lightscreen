# 2026-10-21 (owner decision, before S05): optipng and oxipng are both supported, chosen in the options

**Decision.** The owner decided that the app supports both PNG optimizers as a user choice, not a swap: "If we're going to be targeting multiple OS, and some don't have an oxipng package, both need to be included as options, rather than a simple swap." This replaces the "switch to oxipng" recommendation in `docs/decisions/2026-10-21-oxipng-replaces-optipng.md`; that file's measurements (CLI, size, speed, licences) still hold. The work stays inside S05.

## How the choice is exposed

- **Settings.** `options/optimize` (bool) keeps its meaning and key, so existing settings files are untouched. A new key `options/optimizer` holds `oxipng` or `optipng`. When the key is absent the app picks automatically: oxipng if it is found, else optipng. The key is written only when the user picks a tool in the dialog.
- **Options dialog.** The "optimize" checkbox gets a combo box beside it with "oxipng" and "OptiPNG". An entry whose binary is not found is marked "(not found)" and stays selectable, so a choice made before installing the tool is kept. The checkbox is disabled only when neither tool is found, with the hint "Install oxipng or OptiPNG" (Linux) or "No optimizer found next to lightscreen.exe" (Windows).
- **Defaults per OS.** Windows: oxipng, which the zip ships (see below), so the option works out of the box. Linux: automatic as above; on Ubuntu 24.04, where apt has only optipng, that resolves to optipng.
- **Lookup.** Linux: each tool on `PATH` (`QStandardPaths::findExecutable`). Windows: `oxipng.exe` / `optipng.exe` next to `lightscreen.exe`, as today.
- **Chosen tool missing.** If the chosen tool is not found when a screenshot is optimized, the other one runs if it is found; if neither is, the file is left as it is and `optimizationDone` fires once, as today. The fallback is silent in the capture path (a missing tool already is today); the dialog's "(not found)" label is where the user sees it. Rejected: skipping optimization when the chosen tool is missing (the user would get unoptimized files without noticing, which is the defect this setting exists to avoid).
- **Arguments.** oxipng `-q --out <tmp> <file>`; optipng `-quiet -out <tmp> <file>`. Both at their default level 2; no level setting (not asked for). Same temporary-file-and-replace logic for both; the temporary file is named `<file>.optimize.tmp`.

## What this adds to S05

- The `Optimizer` gets a small tool table (name, binary per OS, argument builder) and a resolve step (choice, then fallback). Roughly one function and a two-entry table more than a single tool would need.
- Options dialog: the combo box (`optionsdialog.ui`, load/save in `optionsdialog.cpp`).
- Tests: the stub becomes a neutral optimizer stub built twice (as `optipng` and `oxipng`) that accepts `-out` and `--out`; `tst_optimize` (or S05's `tst_optimizer`) runs its cases against both names and adds: the chosen tool runs; the chosen tool missing falls back to the other; neither found emits once. `tst_settings`: the new key round-trips, an absent key stays absent, and the "not found" branch checks both binaries. These change safety-net tests, so the S05 PR lists them; the owner merges S05 anyway.
- Linux CI installs optipng from apt and oxipng with `cargo install oxipng --locked` (cached) for one integration test per tool.
- Windows QA (one entry with S05): each tool selected in turn shrinks a capture; with `oxipng.exe` removed from the folder, choosing oxipng still optimizes through optipng and the dialog shows "(not found)".

## Bundling

- **Windows zip ships both**, so either choice works without installing anything: `oxipng.exe` from the oxipng GitHub release (`x86_64-pc-windows-msvc`) and `optipng.exe` from the OptiPNG 0.7.8 Windows release, each downloaded at a pinned version and checked against a SHA-256 recorded in the workflow, in the CI deploy step and in M15. Licence texts for both (zlib for OptiPNG; MIT plus the Apache-2.0 parts for oxipng) go into the zip's third-party notices. Extra size: about 1 to 2 MB for oxipng plus a few hundred kB for optipng (not measured on Windows).
- OptiPNG's Windows binaries are distributed from SourceForge, which is a less dependable download for CI than GitHub releases. If that proves flaky, M15 may mirror the pinned zip as an asset on the fork's own releases; the hash check stays the same.
- **Linux bundles nothing**: both tools come from the user's system (`PATH`), as optipng does today.
- Update check and release flow: unchanged (both exes travel inside the zip; releases stay unsigned).
