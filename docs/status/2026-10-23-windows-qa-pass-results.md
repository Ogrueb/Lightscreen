# 2026-10-23: Windows QA pass, results so far

The owner is running the 22 checks in `docs/windows-qa/` (CI build from `master` at `06db33a`, run #179).

- **Bug 30** (transparent main window): upstream 2.5 only, fixed in the fork by M07; the owner's first report was of the old version and was corrected the same day.
- **New bug 31:** in Windows dark mode the main window uses the `inv/` icons. Upload shows the old Imgur logo and Options a wrench. The owner chose to keep following the theme and to use a generic upload icon. Fixed: the two inverted icons are regenerated from the normal ones, with a new test `tst_Os::invertedIconsMatchTheNormalSet`. `CHANGELOG.md` gains a Dark mode line under Changed. Not listed as a fix, since 2.5 never showed these icons on Windows.
- **A1 (`2026-10-22-rename.md`):** steps 1, 3 and 4 passed. In step 2:
  - The titles passed.
  - "The picker modes show no title" is a mistake in the check, not a bug: the area and window-picker overlays are frameless and have had no title bar since upstream. The check's wording is corrected.
  - Two links to the fork on the About tab: new **bug 32**, fixed (one link, built from `Updater::repository()`).
  - The check file now holds only the About re-check.
