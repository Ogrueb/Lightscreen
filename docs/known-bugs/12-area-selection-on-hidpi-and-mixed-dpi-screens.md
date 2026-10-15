# 12. Area selection on HiDPI and mixed-DPI screens

- **Found:** from upstream pull requests.
- **Where:** `dialogs/areadialog.cpp` (painting at `:488`, `resultRect()` at `:101-108`).
- **What happens:** upstream PR #35 (merged in 2016) fixed the area dialog showing the screenshot magnified by the device pixel ratio. The dialog has been reworked since, and `resultRect()` now scales the selection by the pixmap's single device pixel ratio. On mixed-DPI setups that combines with bug 4, and the selection may not match what ends up in the file. Inferred, needs reproduction.
- **Platform:** Windows with scaling above 100%, especially mixed. **Test:** by hand; select a known region and compare pixel sizes.
