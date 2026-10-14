# Upstream pull requests

The pull requests on the original repo (`ckaiser/Lightscreen`), newest first, as of 2026-10-14. The upstream issue tracker is disabled, so these PRs are the only upstream record of bugs other people hit. The list was read with `gh` and nothing on upstream was touched.

The repo has 10 PRs: 5 open, 2 closed without merging and 3 merged. Notes marked "fork check" compare the PR with this fork's current `master`.

| # | Title | State | Opened | Merged |
|---|-------|-------|--------|--------|
| 44 | Fix Build error | open | 2021-04-21 | no |
| 43 | Typo | open | 2020-01-30 | no |
| 42 | Enable compilation on FreeBSD | closed | 2019-08-10 | no (closed 2019-11-20) |
| 41 | Add lightscreen.spec to enable builds for (S)RPM | open | 2018-04-23 | no |
| 40 | Fix conflicting symbols in linux build | open | 2017-06-14 | no |
| 39 | Fix broken headings in Markdown files | open | 2017-04-17 | no |
| 38 | Readded active monitor capture | closed | 2016-11-04 | no (closed 2019-02-20) |
| 37 | fixed incorrect binding | merged | 2016-08-07 | 2016-08-08 |
| 36 | Generic uploading, imgur and pomf.se clones. | merged | 2016-08-07 | 2016-08-07 |
| 35 | fix rendering of area dialog in hidpi displays | merged | 2016-03-18 | 2016-03-20 |

## #44 Fix Build error (open, +172/-75)

Fixes the Linux build, where enum values in `Screenshot` (`Success`, `None` and others) clash with macros from `X11/X.h`. To do that it turns the `Screenshot` enums into prefixed enum classes (`Mode::M_WholeScreen`, `Result::R_Success`, `Naming::N_Empty`, `Format::F_PNG`) and updates every caller. It also adds a full Qt `.gitignore`, makes qmake put build output under `Build/`, and changes saved-window-position validation from `availableGeometry()` to `screenGeometry(this)`.

Files: `.gitignore`, `dialogs/optionsdialog.cpp`, `lightscreen.pro`, `lightscreenwindow.cpp`, `lightscreenwindow.h`, `tools/screenshot.cpp`, `tools/screenshot.h`.

Fork check: the fork already builds on Linux (see `docs/status.md`), so the clash it fixes no longer blocks the build. The enum-class rename is still a possible clean-up for later.

## #43 Typo (open, +1/-1)

Fixes a one-word typo in the README. No description.

Files: `README.md`.

## #42 Enable compilation on FreeBSD (closed, not merged, +21/-21)

Adds `Q_OS_FREEBSD` next to the Linux checks where the code is X11-specific, and renames the `Success` constant to `SuccessLS` to avoid the X11 clash. The author reports it builds and runs on FreeBSD with Qt 5.12 alongside UGlobalHotkey patches.

Files: `dialogs/optionsdialog.cpp`, `lightscreenwindow.cpp`, `tools/os.h`, `tools/screenshot.cpp`, `tools/screenshot.h`, `tools/windowpicker.cpp`.

## #41 Add lightscreen.spec to enable builds for (S)RPM (open, +187/-0)

Adds RPM packaging: a spec file, a SUSE patch, and a patch that undefines `Success` (the same fix as #40). The author published builds on Fedora COPR and the openSUSE Build Service, and says the app "wont build anywhere" without the `Success` fix.

Files: `pkg/RPM/Lightscreen.spec`, `pkg/RPM/randomdata_suse.patch`, `pkg/RPM/undef_success_x11.patch`.

## #40 Fix conflicting symbols in linux build (open, +1/-0)

Adds `#undef Success` after `#include <X11/X.h>` in `tools/screenshot.cpp`. This fixes the clash between the X11 macro and `Screenshot::Result::Success`.

Files: `tools/screenshot.cpp`.

Fork check: this exact line is already in the fork (`tools/screenshot.cpp:47`), so upstream applied the fix later without merging the PR.

## #39 Fix broken headings in Markdown files (open, +1/-1)

Adds a space after `#` in a README heading so that GitHub's newer Markdown parser renders it as a heading.

Files: `README.md`.

## #38 Readded active monitor capture (closed, not merged, +1/-1)

Bug report as a PR: the "current monitor" option for screen capture grabbed the primary screen instead of the monitor under the cursor. The PR changes `grabDesktop()` from `primaryScreen()->geometry()` to `screenGeometry(QCursor::pos())`.

Files: `tools/screenshot.cpp`.

Fork check: the fork's `grabDesktop()` (`tools/screenshot.cpp:410`) now picks the screen under the cursor, so upstream fixed this another way. It is worth re-testing on a multi-monitor setup, because that code uses the deprecated `QDesktopWidget`.

## #37 fixed incorrect binding (merged, +28/-25)

UGlobalHotkey treats hotkey id 0 as "not a hotkey", but Lightscreen registered the whole-screen action (Print Screen) as id 0, so that hotkey never fired. The PR shifts the hotkey ids so none of them is 0.

Files: `lightscreenwindow.cpp`, `lightscreenwindow.h`, `tools/screenshot.h`.

## #36 Generic uploading, imgur and pomf.se clones. (merged, +1158/-417)

The upstream author's own PR. It adds the generic uploader framework (`tools/uploader/`) with Imgur and pomf.se back ends, plus their options widgets and the dialog changes they need. This is the code the fork's "replace Imgur with a user-configurable uploader" decision builds on.

Files: `dialogs/historydialog.cpp`, `dialogs/optionsdialog.{cpp,h,ui}`, `lightscreen.pro`, `lightscreenwindow.cpp`, `tools/screenshot.{cpp,h}`, `tools/uploader/{imageuploader,imguruploader,pomfuploader,uploader}.{cpp,h}`, `widgets/{imgur,pomf}optionswidget.{cpp,h,ui}`.

## #35 fix rendering of area dialog in hidpi displays (merged, +1/-1)

On HiDPI screens the area-selection dialog showed the screenshot magnified by the device pixel ratio. The PR sets the pixmap's device pixel ratio to the dialog's.

Files: `dialogs/areadialog.cpp`.

Fork check: the area dialog has since been reworked. HiDPI and mixed-DPI area selection is worth re-testing.
