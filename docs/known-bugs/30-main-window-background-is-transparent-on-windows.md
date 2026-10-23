# 30. Main window background is transparent on Windows

- **Found:** by the owner during the Windows QA pass (2026-10-23). First reported against the 3.0 build, then corrected by the owner: the screenshot was of the previous version (upstream 2.5). The 3.0 build (`master` at `06db33a`) draws a solid background.
- **Where:** the main window (`lightscreenwindow.ui`, `LightscreenWindow`).
- **What happens:** the main window's background is transparent. The desktop behind it shows through the margins and gaps between the six buttons, while the buttons themselves and the title bar are drawn normally. The owner's screenshot shows Windows in dark mode.
- **Cause:** the DWM "glass" (`QtWin::enableBlurBehindWindow`). On Windows 10/11 it gives a transparent rather than blurred window, which is why the plan dropped it in M07 (`docs/decisions/2026-10-14-stage-3-shape-of-the-modernization-plan.md`).
- **Platform:** Windows (11 at least), upstream 2.5 only.
- **Status:** fixed in the fork by M07 (Stage 6), which removed the blur. Confirmed by the owner on the 3.0 build (2026-10-23). The 3.0 build looks different in other ways; that is bug 31.
