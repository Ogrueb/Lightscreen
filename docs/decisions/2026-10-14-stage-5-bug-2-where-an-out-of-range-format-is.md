# 2026-10-14 (Stage 5, bug 2): where an out-of-range format is caught

- **Checked twice, one rule:** `Screenshot::formatFromSetting` is used when the window reads `file/format`, and the `Screenshot` constructor passes `mOptions.format` through it too, so any caller that builds `Options` by hand also gets PNG. `extensionFor` keeps its own `.png` default for direct callers. *Rejected:* fixing only `extensionFor` (the file would be named `.png` while `Options::format` still holds the bad value, and the PNG-only paths, quality 80 and optimization, would be skipped).
