# 41. The target folder box suggested no folders

- **Found:** by the owner in the Windows QA pass (A13, `2026-10-15-go-to-folder-and-naming-filter.md` step 3), 2026-10-23, Windows 11: typing a path in Options > Directory showed no suggestions (only the Browse button's folder picker helped).
- **Cause:** M04 replaced Qt 5's `QDirModel`, which Qt 6 removed, with `QFileSystemModel`. That model reads folders in the background. QCompleter matched against folders not read yet, found nothing, and does not complete again once a folder has loaded unless its pop-up was already open. Reproduced on Linux.
- **Fix:** the completer now uses a plain list of the subfolders of the folder typed so far, read when the text changes (as `QDirModel` did), case-insensitive on Windows.
- **Test:** `tst_Settings::targetFolderCompletes` types a path and expects the pop-up with the one matching subfolder, then both subfolders after a bare separator and no `.` or `..` (fails before, passes after).
- **Platform:** all; a regression from M04. **Status:** fixed, awaiting the owner's re-check.
