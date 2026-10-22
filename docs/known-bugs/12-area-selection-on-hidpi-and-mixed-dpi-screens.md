# 12. Area selection on HiDPI and mixed-DPI screens

- **Found:** from upstream pull requests.
- **Where:** `dialogs/areadialog.cpp` (painting at `:488`, `resultRect()` at `:101-108`).
- **What happens:** upstream PR #35 (merged in 2016) fixed the area dialog showing the screenshot magnified by the device pixel ratio. The dialog has been reworked since, and `resultRect()` now scales the selection by the pixmap's single device pixel ratio. On mixed-DPI setups that combines with bug 4, and the selection may not match what ends up in the file. Inferred, needs reproduction.
- **Platform:** Windows with scaling above 100%, especially mixed. **Test:** by hand; select a known region and compare pixel sizes.
- **Status:** fix landed in S04 (Stage 6): the area dialog covers the grabbed desktop's native area, draws it one to one at the dialog's own ratio, and maps the selection back with that ratio (`docs/decisions/2026-10-22-s04-capture-geometry-details.md`). Awaiting the mixed-DPI Windows QA (`docs/windows-qa/2026-10-22-mixed-dpi-capture.md`).
