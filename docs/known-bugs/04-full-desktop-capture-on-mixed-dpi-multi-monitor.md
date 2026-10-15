# 4. Full-desktop capture on mixed-DPI multi-monitor setups

- **Found:** inferred from the code, reproduction needed.
- **Where:** `tools/screenshot.cpp:405-437`, `grabDesktop()`.
- **What happens:** the united geometry of all screens is grabbed through `primaryScreen()->grabWindow(desktop()->winId(), ...)` and tagged with the primary screen's device pixel ratio. When monitors have different scale factors, the secondary monitor is captured at the wrong size or offset, and the cursor overlay lands in the wrong place. The cursor offset arithmetic (`cursorPosition.x() + screenRect.width()` for any screen with a negative left edge) only works when that screen sits exactly at `-width`.
- **Platform:** Windows (Qt 5 with `AA_EnableHighDpiScaling`). **Test:** by hand on a two-monitor setup with different scaling.
- **Upstream:** related to #38 and #35 (see bugs 11-12); test them in the same session.
