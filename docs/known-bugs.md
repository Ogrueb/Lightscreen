# Known bugs

Technical description of each known bug, for Stage 5 (one session per bug marked "fix now"). Line references are to `master` after PR #3. Each entry says where the bug is, how it shows, how to reproduce it, and which platform can reproduce it, since cloud threads can only run Linux.

Draft status: first draft, 2026-10-14. Bugs 1-3 are confirmed from the code; 4-8 are inferred from the code and still need a reproduction. Upstream's GitHub issue tracker could not be read from the cloud thread (API access to `ckaiser/Lightscreen` is not enabled for this session); bugs reported there, and bugs the owner has hit, are still to be added.

## Confirmed from the code

### 1. Window picker icon handle truncated on 64-bit Windows

- **Where:** `tools/windowpicker.cpp:168`, `(HICON)::GetClassLong((HWND)mCurrentWindow, GCL_HICON)`.
- **What happens:** `GetClassLong` returns a 32-bit `DWORD`. On x64 an `HICON` is 64 bits, so the upper half is lost and the cast yields an invalid handle. `QtWin::fromHICON` then gets a bad handle; the picker shows no icon, or a wrong one, for the window under the cursor. MSVC flags it as C4312.
- **Fix:** `GetClassLongPtr(..., GCLP_HICON)`. Consider also `WM_GETICON` / `GCLP_HICONSM` as fallbacks, since many modern windows set no class icon.
- **Platform:** Windows only. **Test:** none possible headless; verify by hand on the owner's machine.

### 2. `Screenshot::extension()` falls off the end for an unknown format

- **Where:** `tools/screenshot.cpp:388-404`; the value comes from `lightscreenwindow.cpp:524`, `(Screenshot::Format) settings()->value("file/format").toInt()`.
- **What happens:** the `switch` covers `PNG`, `BMP`, `WEBP`, `JPEG` (0-3) and there is no `return` after it. A `file/format` value outside 0-3 (hand-edited `config.ini`, a corrupt settings file, or a future enum change) is undefined behavior: typically garbage or a crash when the file name is built. MSVC C4715.
- **Fix:** validate the setting when read (fall back to PNG) and give `extension()` a default branch.
- **Platform:** any. **Test:** headless, `Screenshot` with `format = (Format) 7`.

### 3. Linux error sound file does not exist

- **Where:** `lightscreenwindow.cpp:215`, `QSound::play("sound/ls.error.wav")`.
- **What happens:** on a failed capture with "play sound" enabled, non-Windows builds try to play `sound/ls.error.wav`. The repo has `sounds/` (plural) and it contains only `ls.screenshot.wav`, so nothing plays. Both sound paths are also relative to the current working directory, not the executable or a resource, so even the success sound is silent unless the app is started from its own directory.
- **Fix:** ship both sounds in `lightscreen.qrc` and play from `qrc:/`; pick or draw an error sound.
- **Platform:** Linux for the missing file; both for the relative path. **Test:** headless check that the resource exists.

## Inferred from the code, reproduction needed

### 4. Full-desktop capture on mixed-DPI multi-monitor setups

- **Where:** `tools/screenshot.cpp:405-437`, `grabDesktop()`.
- **What happens:** the united geometry of all screens is grabbed through `primaryScreen()->grabWindow(desktop()->winId(), ...)` and tagged with the primary screen's device pixel ratio. When monitors have different scale factors, the secondary monitor is captured at the wrong size or offset, and the cursor overlay lands in the wrong place. The cursor offset arithmetic (`cursorPosition.x() + screenRect.width()` for any screen with a negative left edge) only works when that screen sits exactly at `-width`.
- **Platform:** Windows (Qt 5 with `AA_EnableHighDpiScaling`). **Test:** by hand on a two-monitor setup with different scaling.

### 5. Window capture margins on Windows 10/11

- **Where:** `tools/os.cpp:171-200`, `os::grabWindow`.
- **What happens:** the window rectangle from `GetWindowRect` is trimmed by `SM_CXSIZEFRAME` and `SM_CXPADDEDBORDER`. Since Windows 10 the visible frame is inside the DWM extended frame, so this heuristic can leave a one-pixel strip of the neighbouring window or cut the window's own edge; `DwmGetWindowAttribute(DWMWA_EXTENDED_FRAME_BOUNDS)` is the documented way to get the visible bounds. Also `GetWindowRect` is in physical pixels while the rest of the code is in Qt logical pixels, which combines with bug 4 on high-DPI screens.
- **Platform:** Windows only.

### 6. Update check and pomf list point at a dead host (and fail silently)

- **Where:** `updater/updater.cpp:44` (`https://lightscreen.com.ar/version`), `widgets/pomfoptionswidget.cpp:92` (`pomf.json`), `lightscreenwindow.cpp:765`, `dialogs/updaterdialog.cpp:50`.
- **What happens:** `Updater::finished` reads the reply without checking `reply->error()`; a failed request parses as version `0` and emits `done(false)`, so "check for updates" reports "up to date" whenever the host is down. The host's state could not be verified from the cloud thread (outbound blocked). The behavior is already decided (GitHub Releases or removal; see `docs/goals.md`), so this entry is here for the "never silently succeed" part.
- **Platform:** any. **Test:** headless with a stub reply.

### 7. SSL errors ignored on "Windows 2003 or older"

- **Where:** `tools/uploader/pomfuploader.cpp:39,126`, `imguruploader.cpp:113,194`, `widgets/imguroptionswidget.cpp:170`, `widgets/pomfoptionswidget.cpp:130`.
- **What happens:** `ignoreSslErrors()` is called when `QSysInfo::WindowsVersion <= WV_2003`. Harmless on supported Windows, but it is six copies of the same dead workaround and goes away with the minimum-Windows decision. Listed so Stage 5 removes it rather than ports it.
- **Platform:** n/a.

### 8. Dead macOS key handler references an undeclared variable

- **Where:** `lightscreenwindow.cpp:1000-1010`, under `#ifdef Q_WS_MAC`.
- **What happens:** `Q_WS_MAC` is a Qt 4 macro, so the block never compiles; if it did, `e->key()` refers to a variable that does not exist. Not a runtime bug; remove with the Qt 6 port.
- **Platform:** n/a.

## Still to add

- Bugs from upstream's issue tracker (`github.com/ckaiser/Lightscreen/issues`), once someone with access lists them.
- Bugs the owner has hit on their own machine.
