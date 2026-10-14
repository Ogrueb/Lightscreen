# Known bugs

Technical description of each known bug, for Stage 5 (one session per bug marked "fix now"). Line references are to `master` after PR #3. Each entry says where the bug is, how it shows, how to reproduce it, and which platform can reproduce it, since cloud threads can only run Linux.

Draft status: first draft, 2026-10-14. Bugs 1-3 are confirmed from the code; 4-8 are inferred from the code and still need a reproduction; 9-10 were reported by the owner on 2026-10-14. Upstream's GitHub issue tracker is disabled, so there is no issue list to mine; the unmerged upstream PRs (see `docs/upstream-prs.md` once it lands) and the owner's own reports are the other sources, still to be added.

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

## Reported by the owner

### 9. Browser windows captured brighter than they appear (HDR display)

- **Where:** all capture modes (desktop, area, window); the capture itself is `QScreen::grabWindow` on the desktop (`tools/screenshot.cpp:436`) or `BitBlt` from the window DC (`tools/os.cpp:171-243`).
- **What happens:** on the owner's setup (monitor in HDR mode, with an AMD driver setting that lowers the SDR baseline brightness), a browser window in the capture is sometimes markedly brighter than on screen. Only browsers are affected, it persists for a few minutes once it starts, and it starts unpredictably. Other windows in the same capture look right.
- **Likely cause (inferred):** browsers composite through a hardware-accelerated swap chain that can run in a different colour space than the desktop (scRGB / 10-bit) while HDR is on. `grabWindow` and `BitBlt` read the SDR-tonemapped desktop surface, and the driver's brightness offset is applied on the display path, not in that surface, so the browser's content is read back before the offset. A capture through the Windows.Graphics.Capture API, or the DXGI desktop duplication API with HDR tonemapping, would see what the display shows.
- **Platform:** Windows, HDR monitor; needs the owner's machine. **Test:** none automated. Mark as "fix if a cheap capture-path change does it, otherwise document".

### 10. PNG optimization: no queue, no bound, no crash safety

- **Where:** `tools/screenshot.cpp:168-200`, `Screenshot::optimize()`, called from `:276-281` after every PNG save when "optimize" is on.
- **What happens:** every screenshot starts its own `optipng` process at once; there is no queue and no concurrency limit, so a burst of captures spikes CPU, and under load a long tail of files stays unoptimized. `optipng` rewrites the file in place, so if the machine shuts down mid-run the screenshot is left half-written next to its backup copy. Two smaller defects in the same function: when the `optipng` binary is missing it emits `optimizationDone()` and then still calls `process->start()`, so `optimizationDone` can fire twice (and on Windows it starts a process that does not exist); the `finished` signal is connected with a string-based `SIGNAL()` whose argument list must match exactly.
- **Fix direction:** one worker queue (`QThreadPool` with one or two slots, or a serial `QProcess` chain) owned by `ScreenshotManager`; have `optipng` write to a temp file and rename over the original only on success (`-out` plus `QFile::rename`), so an interrupted run leaves the original intact; persist the pending list so a restart resumes it; show a count in the tray tooltip.
- **Platform:** any (Linux CI can run `optipng`). **Test:** headless, with a stub executable in place of `optipng`.

## Still to add

- Anything the five unmerged upstream PRs describe (`docs/upstream-prs.md`): #40 Linux conflicting symbols, #44 build error, #38 active-monitor capture.
- Bugs the owner has hit on their own machine.
