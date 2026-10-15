# 23. Area selection loses a pixel at fractional scaling

- **Found:** found while writing the Stage 4 area dialog tests.
- **Where:** `dialogs/areadialog.cpp:101-109`, `AreaDialog::resultRect`.
- **What happens:** the selection's left, top, width and height are each multiplied by the pixmap's device pixel ratio and truncated to `int`. At 150%, the selection (3, 5) 7x9 becomes (4, 7) 10x13 instead of covering device pixels 4.5 to 15 and 7.5 to 21, so the saved image can be a pixel narrower or shorter than what was selected, and the far edge is off by up to one pixel. Integer ratios (100%, 200%) are exact. Related to bug 12; S04 replaces this mapping.
- **Fix:** map the corners (`QRectF` scaled, then `toAlignedRect()` or rounded corners) instead of truncating origin and size separately.
- **Platform:** Windows (or X11 with `QT_SCALE_FACTOR`) at 125%, 150% or 175%. **Test:** `tst_AreaDialog::resultRect` rows "ratio 1.5 truncates" and "ratio 1.25 truncates" record the current values. Confirmed by the test; not reproduced in the running app.
- **Triage:** deferred to S04 (replaces the mapping); its done-check flips the fractional rows.
