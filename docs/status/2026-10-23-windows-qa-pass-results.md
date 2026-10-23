# 2026-10-23: Windows QA pass, results so far

The owner is running the 22 checks in `docs/windows-qa/`. A1 to A6 ran on the CI build from `master` at `06db33a` (run #179), which has none of the fixes below; each fix is re-checked with a PR #58 build (`27bffb3` or later).

- **Bug 30** (transparent main window): upstream 2.5 only, fixed in the fork by M07; the owner's first report was of the old version and was corrected the same day.
- **New bug 31:** in Windows dark mode the main window uses the `inv/` icons. Upload shows the old Imgur logo and Options a wrench. The owner chose to keep following the theme and to use a generic upload icon. Fixed: the two inverted icons are regenerated from the normal ones, with a new test `tst_Os::invertedIconsMatchTheNormalSet`. `CHANGELOG.md` gains a Dark mode line under Changed. Not listed as a fix, since 2.5 never showed these icons on Windows.
- **A1 (`2026-10-22-rename.md`):** steps 1, 3 and 4 passed. In step 2:
  - The titles passed.
  - "The picker modes show no title" is a mistake in the check, not a bug: the area and window-picker overlays are frameless and have had no title bar since upstream. The check's wording is corrected.
  - Two links to the fork on the About tab: new **bug 32**, fixed (one link, built from `Updater::repository()`).
  - The check file now holds only the About re-check.
- **A2 (`2026-10-22-fresh-install.md`):** steps 1 and 2 passed; step 4 (clean up) not applicable. In step 3:
  - With Windows' Print Screen setting on, the Snipping Tool takes the key. Recorded as a known issue in `CHANGELOG.md` and the release notes.
  - With it off, Print Screen captures only while a Lightscreen Rd window or menu is open, not from the tray: new **bug 33**, under investigation (Print Screen only, or every hotkey in the background).
  - The check file now holds only the re-check.
- **Bug 33 narrowed down and fixed:** the owner found every hotkey fails from the tray while upstream 2.5 works, so it is a Qt 6 regression. `UGlobalHotkeys` on Windows now registers hotkeys to the thread and catches `WM_HOTKEY` with a native event filter, so no window is involved. New test suite `tst_globalhotkeys` (Windows), 13 suites in total. Cause inferred, fix awaiting the owner's re-check.
- **A4 (`2026-10-21-area-capture-after-s01.md`):** all six steps passed.
  - "Green tick" was the check's wording; the accept button is a plain tick, as in 2.5.
  - Step 4 gave 125x126 for a typed 100x100 at 125%. The owner chose exact sizes over covering every touched pixel: `CaptureGeometry::selectionToDevice` now rounds the edges and the size. Safety-net rows in `tst_AreaDialog::resultRect` and `tst_CaptureGeometry::selectionToDevice` changed on purpose ("rounds"), with typed-size rows added; bug 23's file records it.
  - Verified under Xvfb at 1, 1.25 and 1.5. The check file holds only the 125%/150% re-check.
- **A5 (`2026-10-15-hotkey-from-keyboard.md`):** steps 1 and 2 passed; step 3 found **bug 34**, fixed: a plain Tab or Shift+Tab pressed while a hotkey button showed "Type your hotkey" was recorded as the hotkey. It now ends the entry and moves focus on. New `tst_HotkeyWidget` cases (fail before, pass after). The check file holds the re-check, with the highlighted and entry-mode cases split.
- **A6 (`2026-10-14-window-capture-bounds.md`):** steps 1 and 2 passed. Step 3 (window partly off screen: Explorer's title bar black) and the window picker (title bar showing the window behind) are **bug 35**, fixed: `os::grabWindow` now has Windows draw the window with `PrintWindow(PW_RENDERFULLCONTENT)` instead of copying screen or window-DC pixels, and releases its DCs. New `tst_Os::grabWindowPartlyOffScreen` (Windows). The check file holds the re-check.
- **A3 (`2026-10-20-qt6-flip.md`):** steps 1, 3, 4, 5 and 6 passed; 7 (portable mode) passed through the fresh-install check; 8 (autostart) skipped.
  - In step 2, the window picker captured the window behind as well: bug 35's symptom, fixed in the PR #58 build at `27bffb3`, so it is a re-check.
  - Active-window capture worked with the Lightscreen window open; from the tray it depends on bug 33's fix, also a re-check.
  - The check file holds the two re-checks and autostart.
- **A7 (`2026-10-14-window-picker-icons.md`):** steps 1 and 3 passed. Step 2 passed except Settings, a Store app, which shows no icon: **bug 36**, a known issue for 3.0 (owner). The picker's title-bar clipping seen in A7 is bug 35, on the A1 build. The check file is removed.
- **A8 (`2026-10-16-taskbar-without-winextras.md`):** steps 2, 4 and 5 passed; 3 is covered by A7; 1 (upload progress) skipped.
  - The cross is not red because the overlay is the monochrome icon; the check's wording is corrected.
  - Step 6 showed Lightscreen's tray-icon menu, not a jump list. The check now says to right-click the taskbar button itself.
  - Re-checked: the taskbar button's own menu shows only Windows' standard items, so step 6 passes.
  - The check file holds step 1 (upload progress).
- **A9 (`2026-10-15-sound-cues.md`):** step 2 passed. Step 1: the shutter played together with Windows' notification sound. The owner confirmed the second sound is the tray pop-up's (only the shutter with the pop-up off). Documented as **bug 37**, a known issue with a workaround (owner's choice), in `CHANGELOG.md` and the release notes. The check file is removed.
- **A10 (`2026-10-22-quit-with-close-hide.md`):** passed; the check file is removed.
