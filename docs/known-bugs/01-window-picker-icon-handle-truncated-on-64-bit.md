# 1. Window picker icon handle truncated on 64-bit Windows

- **Found:** confirmed from the code.
- **Where:** `tools/windowpicker.cpp:168`, `(HICON)::GetClassLong((HWND)mCurrentWindow, GCL_HICON)`.
- **What happens:** `GetClassLong` returns a 32-bit `DWORD`. On x64 an `HICON` is 64 bits, so the upper half is lost and the cast yields an invalid handle. `QtWin::fromHICON` then gets a bad handle; the picker shows no icon, or a wrong one, for the window under the cursor. MSVC flags it as C4312.
- **Fix:** `GetClassLongPtr(..., GCLP_HICON)`. Consider also `WM_GETICON` / `GCLP_HICONSM` as fallbacks, since many modern windows set no class icon.
- **Platform:** Windows only. **Test:** none possible headless; verify by hand on the owner's machine.
- **Status:** fixed in Stage 5 (`GetClassLongPtr(GCLP_HICON)`, then `GCLP_HICONSM`, then `WM_GETICON`/`ICON_BIG`); awaiting the owner's Windows QA.
