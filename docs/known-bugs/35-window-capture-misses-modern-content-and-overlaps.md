# 35. Window capture shows black title bars, or the window behind

- **Found:** by the owner in the Windows QA pass (A6, `2026-10-14-window-capture-bounds.md`), 2026-10-23, Windows 11. Not specific to Explorer: other apps show the same, with different areas wrong.
- **Where:** `os::grabWindow` (`tools/os.cpp`), used by active-window capture and the window picker.
- **What happens:**
  1. A window partly off screen (step 3): the title bar and toolbar area of Explorer comes out black.
  2. The window picker: the title bar area shows the window behind instead of the picked window.
- **Cause:**
  - `os::grabWindow` had two ways to copy the pixels, and neither sees everything a Windows 10/11 app draws.
  - A window fully on screen was brought to the top and copied from the screen with `BitBlt`. That copy takes whatever is on screen at that instant: the picker calls it right after closing itself, before the desktop has redrawn, so the picker's overlay or the window behind shows through.
  - A window partly off screen was copied from its own window DC. Content drawn through DirectComposition (the title bars and toolbars of current apps such as Explorer, and browser content) is not in that DC, so it is black.
- **Fix:** `PrintWindow(hwnd, ..., PW_RENDERFULLCONTENT)` (Windows 8.1 and later). Windows draws the window itself, DirectComposition included, whatever covers it and wherever it is. The result is cropped from the `GetWindowRect` bitmap to the DWM visible frame (bug 5). If `PrintWindow` fails, the area is copied from the screen as a fallback. The old code also never released the screen or window DC and called `ReleaseDC` on the memory DC; both are fixed.
- **Known limits:**
  - On Windows 11 the four rounded corners lie outside the window's shape, and what `PrintWindow` puts there is not defined (likely black or white).
  - An app that draws with exclusive full-screen or protected content (some games, DRM video) may still capture black.
- **Test:** `tst_Os::grabWindowPartlyOffScreen` (Windows) creates a plain window 120 pixels off the left of the desktop and checks the capture's size (the DWM frame) and its colour in the middle and in the off-screen part. A plain GDI window cannot show the DirectComposition case; the owner's re-check covers that.
- **Platform:** Windows 10/11. **Status:** fixed (2026-10-23), awaiting the owner's re-check.
