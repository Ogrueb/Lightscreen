# 2026-10-15: Stage 6, M12 done (Imgur removed, PR #35)

- Deleted the Imgur uploader and its options widget; the options dialog's upload tab lists one service (pomf, until S03) and has no Imgur page. Menus, tooltips and the history-checkbox help no longer mention Imgur; the `imgur` icon alias and PNGs are now `upload` (same images), and the main window's button is `uploadPushButton`.
- `Uploader::serviceName` returns `"pomf"` for every index and `ImageUploader::factory` returns the pomf uploader for any name, so an unknown name no longer crashes (bug 21, `tst_uploader::uploadUnknownService`). The history dialog's upload reads `upload/service` through `serviceName` (bug 14).
- History stores the delete URL as given (no `https://imgur.com/delete/` prefix); "Delete from imgur.com" is "Open delete link", enabled only when the row has one.
- Settings: `upload/service` values the one-entry box does not list load as 0 (pomf), so a pre-M12 pomf setting (1) keeps working; `upload/imgur/*` keys stay in the file untouched (`tst_settings::dialogLegacyImgurSettings`). Goldens lose the `upload/imgur/*` lines; the round trip saves `upload/service=0`; `dialogOutOfRangeIndexes` saves 0 instead of -1.
- `grep -ri imgur` outside `docs/` finds only the README's history line and test lines that feed old Imgur settings. All 10 suites pass on Linux under Xvfb.
- Windows QA: `docs/windows-qa/2026-10-15-imgur-removed.md` (optional, short).
