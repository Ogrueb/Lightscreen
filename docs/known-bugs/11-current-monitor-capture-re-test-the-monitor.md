# 11. "Current monitor" capture: re-test the monitor under the cursor

- **Found:** from upstream pull requests.
- **Where:** `tools/screenshot.cpp:410-414`, `grabDesktop()` when `currentMonitor` is set.
- **What happens:** upstream PR #38 reported that this option captured the primary monitor instead of the one under the cursor. The current code picks the screen via `qApp->desktop()->screenNumber(QCursor::pos())`, so the original report looks fixed. That is inferred, not reproduced. `QDesktopWidget` is removed in Qt 6, so the Qt 6 port will rewrite this path anyway (`QGuiApplication::screenAt`).
- **Platform:** any multi-monitor setup. **Test:** by hand with the cursor on a non-primary monitor; after the port, a check that `screenAt` drives the choice.
