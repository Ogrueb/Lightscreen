# 30. Main window background is transparent on Windows

- **Found:** by the owner during the Windows QA pass (2026-10-23), with the CI build from `master` at `06db33a`. Upstream Lightscreen 2.5 shows the same.
- **Where:** the main window (`lightscreenwindow.ui`, `LightscreenWindow`).
- **What happens:** the main window's background is transparent. The desktop behind it shows through the margins and gaps between the six buttons, while the buttons themselves and the title bar are drawn normally. The owner's screenshot shows Windows in dark mode.
- **Likely cause (inferred, not confirmed):**
  - Upstream: the DWM "glass" (`QtWin::enableBlurBehindWindow`), which on Windows 10/11 gives a transparent rather than blurred window. That is why the plan dropped it in M07 (`docs/decisions/2026-10-14-stage-3-shape-of-the-modernization-plan.md`).
  - In the fork: that call is gone, and nothing in the code sets a translucent background, a window opacity or a stylesheet on the main window. The likely source is Qt 6.8's `windows11` style, the default on Windows 11 since Qt 6.7, and the way it draws (or leaves out) the `QMainWindow` background, possibly only in dark mode.
- **To pin down:** start with `-style windowsvista` and then `-style fusion`. Try Windows light mode. Try Windows 10 if one is at hand. If the background is solid in any of these, the style is the cause.
- **Fix direction, depending on the above:**
  - give the central widget a solid background (`setAutoFillBackground(true)` with the palette's `Window` colour), or
  - choose the style explicitly in `main.cpp` (`windowsvista` or `fusion`) if `windows11` misbehaves elsewhere too.
- **Also seen in the screenshot (cosmetic, not part of this bug):** the 260-pixel-wide window cuts its title to "Light…".
- **Platform:** Windows (11 at least). **Test:** by hand only. Add the check to `docs/windows-qa/` with the fix.
- **Status:** open, logged during the QA pass; not yet triaged. The QA pass continues (A1 results pending).
