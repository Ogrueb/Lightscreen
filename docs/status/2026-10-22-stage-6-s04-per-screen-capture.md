# 2026-10-22: Stage 6, S04 done (per-screen capture and DPI-correct area selection)

- **New `tools/capturegeometry.{h,cpp}`**: pure mapping functions over synthetic screens `{logical geometry, ratio, native size}`, tested by the new `tests/tst_capturegeometry` (layouts: 100% next to 150%, a screen at a negative offset, a 200% screen above the primary; fractional ratios; a selection across two screens).
- **`Screenshot::grabDesktop`** grabs each `QScreen` (or only the one under the cursor) at its own ratio and composes the grabs on one canvas in native pixels; the cursor goes through its own screen's ratio. New accessor `Screenshot::deviceGeometry()`.
- **`AreaDialog`** gets the canvas's native rectangle (new optional constructor argument, passed by `LightscreenWindow::selectArea`), covers exactly that native area, draws the canvas at its own ratio and maps the selection back with it; `resultRect` now covers every device pixel the selection touches (bug 23) and is clipped to the pixmap; the magnifier copies the right device pixels at ratios other than 1.
- **`os::grabWindow`** (Windows) compares against the whole virtual desktop, so a window on a secondary monitor takes the screen-DC path (bug 5, remaining part).
- **Deviation from the plan:** the selection is mapped with the dialog window's single ratio, not per screen in logical coordinates, because that is how Qt 6 maps a window's coordinates. Reasons and details: `docs/decisions/2026-10-22-s04-capture-geometry-details.md`.
- **Safety-net changes (on purpose):** `tst_AreaDialog::resultRect` rows "ratio 1.5 truncates" and "ratio 1.25 truncates" became "covers every pixel" with `QRect(4, 7, 11, 14)` and `QRect(1, 1, 4, 4)` (were `QRect(4, 7, 10, 13)` and `QRect(1, 1, 3, 3)`); new rows "ratio 1.75, far edge" and "ratio 1, clipped" (a selection past the pixmap is now clipped to it). All other expectations unchanged.
- **Local check** (Ubuntu 24.04, apt Qt 6.4): app and tests build; all 12 suites pass under Xvfb; `tests/check-includes.sh` passes. End to end under Xvfb (one 1600x900 screen, whole-screen on Print and area with autoclose on F9, area dragged over native pixels 301-602 x 101-403):

  | `QT_SCALE_FACTOR` | whole screen, `master` | whole screen, S04 | area, `master` | area, S04 |
  |---|---|---|---|---|
  | 1 | 1600x900 | 1600x900 | 302x303 | 302x303 |
  | 1.5 | 1601x900 | 1600x900 | 301x304 | 302x305 |
  | 2 | 1600x900 | 1600x900 | 302x304 | 302x304 |

  At 1.5 `master` adds a column to the whole-screen grab and loses one from the area (bug 23); S04 does neither. The sizes at 1.5 and 2 can differ from 100% by a pixel because the mouse moves in logical pixels.
- **Not checked here:** two screens. Xvfb gives Qt only one (Qt's xcb backend ignores Xinerama; Xvfb has no second RandR output), so mixed DPI is checked only by the unit tests and by hand: `docs/windows-qa/2026-10-22-mixed-dpi-capture.md`. The Windows code (`os::grabWindow`) is compiled by CI only.
- **Bugs:** 23 fixed; 4 and 12 fix landed, awaiting the mixed-DPI QA; 11 re-test folded into the same QA file; 5 finished in code, awaiting QA. Each bug file has a new **Status:** line.
- **Next:** S05 (optimizer queue) can start once this merges (both touch `tools/screenshot.cpp`). The Windows QA pass suggested in `2026-10-22-remaining-work-overview.md` can run after this PR, since it adds the mixed-DPI checks.
