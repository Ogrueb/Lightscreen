# 15. Collision naming can pick an existing file, which is then overwritten

- **Found:** found by the Stage 4 screenshot naming tests.
- **Where:** `tools/screenshot.cpp`, `Screenshot::uniqueName` (called from `Screenshot::save`).
- **What happens:** for each file containing the name, it removes *every* occurrence of the name, `" ("` and `")"`, then parses what is left. With the name `2` and `2.png`, `2 (1).png`, `2 (2).png` on disk, `2 (2).png` reads as 0 (its `2` inside the suffix is removed too), the maximum is 1, and it returns `2 (2)`. `save()` does not check again and writes over `2 (2).png`. Any name containing digits can hit this (Numeric naming produces such names with `replace` off whenever a file already has the next number).
- **Fix:** parse only an exact `<name> (<n>)<extension>` pattern, or loop until the candidate does not exist.
- **Platform:** any. **Test:** `tst_screenshot::uniqueName_returnsExistingName` (confirmed).
- **Triage:** fix now (Stage 5 session C, with 17); priority: data loss.
- **Status:** fixed in Stage 5 (2026-10-14), with 17. `uniqueName` counts only files named exactly `<name> (<n>)<extension>` and then counts up until `<name> (<n>)<extension>` does not exist. Tests: `uniqueName_returnsExistingName` (now `2 (3)`), `uniqueName_skipsExistingCandidate`.
