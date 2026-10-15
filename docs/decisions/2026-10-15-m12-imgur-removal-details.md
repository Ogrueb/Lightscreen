# 2026-10-15 (M12): details of the Imgur removal the plan left open

- **The factory always returns pomf, and `Uploader::upload` has no null check.** With the fallback the factory cannot return `nullptr`, so a guard would be dead code; S03, which rewrites the factory, keeps the `uploadUnknownService` test. *Rejected:* both (unreachable code).
- **An out-of-range `upload/service` is clamped to 0 when the dialog loads**, not kept as is like `file/format` (bug 19): pomf is the only choice, and every index maps to it anyway. *Rejected:* keeping 1 in the file (a downgrade would then pick pomf, but the dialog would show nothing selected).
- **The image files are renamed too** (`images/upload.png`, `images/inv/upload.png`), not only the alias, so nothing outside `docs/` and the legacy-settings tests says Imgur.
- **Tests keep the string `imgur` where they prove old settings still load**; the plan's `grep -ri imgur` done-check is read as "no Imgur code or UI left".
