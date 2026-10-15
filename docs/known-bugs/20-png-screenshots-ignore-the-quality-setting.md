# 20. PNG screenshots ignore the quality setting

- **Found:** found by the Stage 4 settings tests.
- **Where:** `tools/screenshot.cpp:57-58`, the `Screenshot` constructor.
- **What happens:** for PNG it replaces `options/quality` with 80 before saving. For PNG, Qt maps the quality value to zlib compression (lossless), so the effect is a fixed compression level, not lower image quality. The options dialog greys out the quality slider for PNG, so this matches the UI; it is listed so the behaviour is a decision, not an accident.
- **Fix:** none needed unless PNG compression should become configurable; S05 (PNG optimization) is the natural place to decide.
- **Platform:** any. **Test:** `tst_settings::windowPngQualityIsForced` (Linux, under Xvfb). Confirmed by the test.
- **Triage:** no change; revisit only if S05 makes PNG compression configurable.
