# 32. The About tab links the fork's GitHub page twice

- **Found:** by the owner in the Windows QA pass (A1, `2026-10-22-rename.md` step 2), 2026-10-23.
- **Where:** `OptionsDialog::init` (`dialogs/optionsdialog.cpp`) and the `linksLabel` text in `dialogs/optionsdialog.ui`.
- **What happens:** the About tab shows "GitHub page" and "Lightscreen Rd on GitHub", both linking to `Ogrueb/Lightscreen-Rd`. M13 prepends "GitHub page" in code, built from `Updater::repository()`. M16 then added the fork's link to the `.ui` text as well.
- **Fix:** the `.ui` keeps only the link to the original. The code prepends "Lightscreen Rd on GitHub · " from `Updater::repository()`, so the fork's address has one source, the same one the update check uses.
- **Platform:** any. **Test:** by hand (`docs/windows-qa/2026-10-22-rename.md`).
- **Status:** fixed (2026-10-23), awaiting the owner's re-check.
