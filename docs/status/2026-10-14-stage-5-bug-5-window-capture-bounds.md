# 2026-10-14: Stage 5, bug 5 window capture bounds

- `os::grabWindow` (Windows only) now takes the window's visible bounds from `DwmGetWindowAttribute(DWMWA_EXTENDED_FRAME_BOUNDS)`. If that call fails, it falls back to `GetWindowRect` with the old `SM_CXSIZEFRAME`/`SM_CXPADDEDBORDER` trimming, unchanged. `dwmapi` is linked for MSVC and MinGW in `src.pri`.
- The window-DC path (a window partly off screen) now copies from the visible frame's offset inside the window DC instead of from `0,0`. Before, it took the invisible left/top border and cut the same amount off the right and bottom.
- DPI is untouched (S04). Both rectangles are physical pixels in a per-monitor-aware process, so they agree with each other.
- Not verified here: cloud threads build Linux only, and this code is `#ifdef Q_OS_WIN`. Windows CI builds it. Windows QA by the owner is still open: capture a non-maximized window next to another (no stripe of the neighbour, no cut edge), a maximized one, and one partly off screen.
