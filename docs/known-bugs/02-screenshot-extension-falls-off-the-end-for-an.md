# 2. `Screenshot::extension()` falls off the end for an unknown format

- **Found:** confirmed from the code.
- **Where:** `tools/screenshot.cpp:388-404`; the value comes from `lightscreenwindow.cpp:524`, `(Screenshot::Format) settings()->value("file/format").toInt()`.
- **What happens:** the `switch` covers `PNG`, `BMP`, `WEBP`, `JPEG` (0-3) and there is no `return` after it. A `file/format` value outside 0-3 (hand-edited `config.ini`, a corrupt settings file, or a future enum change) is undefined behavior: typically garbage or a crash when the file name is built. MSVC C4715.
- **Fix:** validate the setting when read (fall back to PNG) and give `extension()` a default branch.
- **Platform:** any. **Test:** headless, `Screenshot` with `format = (Format) 7`.
- **Status:** fixed in Stage 5. `Screenshot::formatFromSetting` reads `file/format` and falls back to PNG for anything outside 0-3; the `Screenshot` constructor applies the same rule and `extensionFor` returns `.png` by default. Tests: `tst_screenshot::extensionFor_outOfRange`, `formatFromSetting`, `save_outOfRangeFormat`.
