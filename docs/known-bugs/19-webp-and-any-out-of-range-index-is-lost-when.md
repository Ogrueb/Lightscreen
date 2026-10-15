# 19. WEBP (and any out-of-range index) is lost when the options dialog is saved

- **Found:** found by the Stage 4 settings tests.
- **Where:** `dialogs/optionsdialog.cpp:180` loads `file/format` into `formatComboBox`, which lists only PNG, JPG and BMP; `:332` saves `currentIndex()` back. The same load/save pattern applies to `file/naming`, `options/previewPosition` and `upload/service`.
- **What happens:** `Screenshot::Format` has a fourth value, WEBP (3), which the main window accepts. `QComboBox::setCurrentIndex(3)` on a three-item box leaves no current item, so opening the options and saving them writes `file/format=-1`. The window then passes format `-1` to `Screenshot`, which is the out-of-range case of bug 2 (inferred from the code, not exercised).
- **Fix:** add WEBP to the combo box, and clamp or keep unknown indexes on load instead of saving `-1`.
- **Platform:** any. **Test:** `tst_settings::dialogOutOfRangeIndexes` asserts the current `-1`; it flips when this is fixed. Confirmed by the test.
- **Triage:** fix now (session E), unless the bug 2 PR already covers it.
- **Since the bug 2 fix:** the saved `-1` is read back as PNG, so there is no undefined behaviour any more, but a WEBP choice still silently becomes PNG. The dialog part stays open.
- **Status:** fixed in Stage 5 (2026-10-15). The format combo box lists WEBP, and `saveSettings` keeps the stored `file/format`, `file/naming` and `options/previewPosition` when the box has no current item instead of writing `-1`. `upload/service` still saves `-1`, left to M12. `tst_settings::dialogOutOfRangeIndexes` flipped.
