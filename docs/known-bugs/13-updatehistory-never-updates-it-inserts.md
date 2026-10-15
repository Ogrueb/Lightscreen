# 13. `updateHistory` never updates, it inserts

- **Found:** found while writing the Stage 3 plan.
- **Where:** `tools/screenshotmanager.cpp:117-137`, `ScreenshotManager::updateHistory`.
- **What happens:** the lookup is `SELECT fileName FROM history WHERE URL IS NOT EMPTY AND fileName = ?`. `EMPTY` is not SQLite syntax, so `prepare` fails, `query.record().count()` is 0, and the function falls through to `saveHistory`, which inserts a second row for the file instead of filling in its URL. Visible as duplicate rows in the history dialog after uploading a screenshot that was already recorded.
- **Fix:** `WHERE fileName = ?` (and decide whether an existing URL should be overwritten), then test `exec()` and `next()` instead of `record().count()`.
- **Platform:** any. **Test:** headless, save a file then upload it, expect one row with the URL. Confirmed by the Stage 4 history test. **Status: fixed in Stage 5 (2026-10-14).** The lookup is `WHERE fileName = ?`, checked with `exec()` and `next()`; an existing URL is overwritten by the newer upload; an empty delete hash now stores an empty delete URL, as `saveHistory` does. `tst_history` asserts one row with the URL.
