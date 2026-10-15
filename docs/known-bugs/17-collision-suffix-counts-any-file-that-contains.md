# 17. Collision suffix counts any file that contains the name (lower severity)

- **Found:** found by the Stage 4 screenshot naming tests.
- **Where:** `Screenshot::uniqueName`.
- **What happens:** `file.contains(name)` matches longer names, so `screenshot1` next to `screenshot12.png` becomes `screenshot1 (3)`, and `shot (1) (1).png` becomes `shot (12)`. Odd numbers, no data loss on its own (see 15 for the case that loses data).
- **Fix:** the same pattern match as 15.
- **Platform:** any. **Test:** `tst_screenshot::uniqueName_prefixMatch`, `tst_screenshot::uniqueName_nestedSuffix`.
- **Triage:** fix now, with 15 (session C).
- **Status:** fixed in Stage 5 (2026-10-14), with 15: the exact pattern match ignores longer names and nested suffixes (`screenshot1 (1)`, `shot (1)`).
