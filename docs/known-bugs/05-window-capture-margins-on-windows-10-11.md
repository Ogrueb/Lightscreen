# 5. Window capture margins on Windows 10/11

- **Found:** inferred from the code, reproduction needed.
- **Where:** `tools/os.cpp:171-200`, `os::grabWindow`.
- **What happens:** the window rectangle from `GetWindowRect` is trimmed by `SM_CXSIZEFRAME` and `SM_CXPADDEDBORDER`. Since Windows 10 the visible frame is inside the DWM extended frame, so this heuristic can leave a one-pixel strip of the neighbouring window or cut the window's own edge; `DwmGetWindowAttribute(DWMWA_EXTENDED_FRAME_BOUNDS)` is the documented way to get the visible bounds. Also `GetWindowRect` is in physical pixels while the rest of the code is in Qt logical pixels, which combines with bug 4 on high-DPI screens.
- **Platform:** Windows only.
- **Status:** bounds fixed in Stage 5 (`os::grabWindow` asks DWM for `DWMWA_EXTENDED_FRAME_BOUNDS` and keeps the old trimming only as the fallback); awaiting Windows QA. The physical-vs-logical pixel part stays with S04.
- **S04:** `os::grabWindow` now compares the window with the whole virtual desktop (`SM_XVIRTUALSCREEN` and friends) instead of `GetDesktopWindow()`, which covers only the primary monitor, so a window on another monitor is grabbed from the screen DC like one on the primary. All of `os::grabWindow` works in physical pixels (Qt 6 makes the process per-monitor DPI aware), so no logical-to-physical mapping is needed there. Awaiting Windows QA.
