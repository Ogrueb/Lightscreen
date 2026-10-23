# 40. The date format box took characters a file name cannot hold

- **Found:** by the owner in the Windows QA pass (A13, `2026-10-15-go-to-folder-and-naming-filter.md` step 2), 2026-10-23, Windows 11: `?`, `:` and `*` could be typed. The error pop-up the owner saw on saving is the options dialog's check of the file-name prefix ("Filename character error"). The date format, set in the Naming dialog, had no such check, so a format with `:` gave file names Windows refuses.
- **Cause:** `NamingDialog` filtered keys with an event filter on the editable combo box, but the typing goes to the box's own text field, so the filter never saw a key. The code is upstream's (M04 only replaced `QRegExp`), so 2.5 very likely has the same bug.
- **Fix:** a `QRegularExpressionValidator` on the box's text field, without `? : \ / * " < > |` on Windows and `/` elsewhere. It also stops pasted text, which the key filter never could. The event filter is removed.
- **Test:** `tst_Settings::dateFormatRejectsFileNameCharacters` types into the text field (fails before, passes after).
- **Platform:** all (the Windows character set on Windows). **Status:** fixed, awaiting the owner's re-check.
