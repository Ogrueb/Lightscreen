# 2026-10-15: Stage 5, bug 19 fixed (WEBP lost by the options dialog)

- The bug 2 fix (PR #22) only made a saved `-1` read back as PNG, so a WEBP choice still became PNG after opening and saving the options. The format combo box now lists WEBP (index 3, matching `Screenshot::WEBP`), and `OptionsDialog::saveSettings` leaves `file/format`, `file/naming` and `options/previewPosition` untouched when the box has no current item (a stored index it does not list) instead of writing `-1`.
- `tst_settings::dialogOutOfRangeIndexes` flipped (fails before the fix): WEBP shows as "WEBP" and saves as 3, naming and preview position 9 survive. `upload/service` still saves `-1`, left to M12. Goldens unchanged. All suites pass on Linux under Xvfb.
- Windows QA (optional, short): choose WEBP, take a screenshot, check the `.webp` file opens. Writing WEBP needs Qt's `qwebp` image format plugin, which `windeployqt` deploys when the Qt install has it.
