# Known bugs

Technical description of each known bug, for Stage 5 (one session per bug marked "fix now"). Line references are to `master` after PR #3. Each entry says where the bug is, how it shows, how to reproduce it, and which platform can reproduce it, since cloud threads can only run Linux.

Draft status: second draft, 2026-10-14. Bugs 1-3 are confirmed from the code; 4-8 are inferred from the code and still need a reproduction; 9-10 were reported by the owner on 2026-10-14; 13-14 were found while writing the Stage 3 plan; 15-18 by the Stage 4 screenshot naming tests; 19-20 by the Stage 4 settings tests; 21-22 while writing the Stage 4 uploader tests; 23-24 while writing the Stage 4 area dialog tests; 25 by the Stage 4 os helpers tests; 26 by the Stage 4 hotkey widget tests; 27-29 by the Stage 4 main window tests. The Stage 5 triage (fix now or defer, and where) is in `docs/modernization-plan.md`, section 7. Upstream's GitHub issue tracker is disabled, so there is no issue list to mine; bugs 11-12 come from the upstream pull requests (`docs/upstream-prs.md`), and 9-10 from the owner.

## Confirmed from the code

### 1. Window picker icon handle truncated on 64-bit Windows

- **Where:** `tools/windowpicker.cpp:168`, `(HICON)::GetClassLong((HWND)mCurrentWindow, GCL_HICON)`.
- **What happens:** `GetClassLong` returns a 32-bit `DWORD`. On x64 an `HICON` is 64 bits, so the upper half is lost and the cast yields an invalid handle. `QtWin::fromHICON` then gets a bad handle; the picker shows no icon, or a wrong one, for the window under the cursor. MSVC flags it as C4312.
- **Fix:** `GetClassLongPtr(..., GCLP_HICON)`. Consider also `WM_GETICON` / `GCLP_HICONSM` as fallbacks, since many modern windows set no class icon.
- **Platform:** Windows only. **Test:** none possible headless; verify by hand on the owner's machine.
- **Status:** fixed in Stage 5 (`GetClassLongPtr(GCLP_HICON)`, then `GCLP_HICONSM`, then `WM_GETICON`/`ICON_BIG`); awaiting the owner's Windows QA.

### 2. `Screenshot::extension()` falls off the end for an unknown format

- **Where:** `tools/screenshot.cpp:388-404`; the value comes from `lightscreenwindow.cpp:524`, `(Screenshot::Format) settings()->value("file/format").toInt()`.
- **What happens:** the `switch` covers `PNG`, `BMP`, `WEBP`, `JPEG` (0-3) and there is no `return` after it. A `file/format` value outside 0-3 (hand-edited `config.ini`, a corrupt settings file, or a future enum change) is undefined behavior: typically garbage or a crash when the file name is built. MSVC C4715.
- **Fix:** validate the setting when read (fall back to PNG) and give `extension()` a default branch.
- **Platform:** any. **Test:** headless, `Screenshot` with `format = (Format) 7`.
- **Status:** fixed in Stage 5. `Screenshot::formatFromSetting` reads `file/format` and falls back to PNG for anything outside 0-3; the `Screenshot` constructor applies the same rule and `extensionFor` returns `.png` by default. Tests: `tst_screenshot::extensionFor_outOfRange`, `formatFromSetting`, `save_outOfRangeFormat`.

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
- **Upstream:** related to #38 and #35 (see bugs 11-12); test them in the same session.

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
- **Status:** local part fixed in Stage 5 (`tests/tst_optimize`). `optimizationDone` runs exactly once per call: the missing-binary branch returns, a process that fails to start is handled through `errorOccurred`, and `finished` is connected pointer-to-member. `optipng -out <file>.optipng.tmp <file>` writes beside the original, which is replaced (`rename` / `MoveFileEx`) only on exit code 0; on failure the temporary file is removed. Also fixed: on Linux the binary check was `QFile::exists("optipng")`, a path relative to the working directory, so it almost always failed and `finished` fired once at once and again when `optipng` exited; it is now `QStandardPaths::findExecutable`. Open for S05: queue, concurrency bound, persisted pending list, tray count, and cleaning up a `.optipng.tmp` left by a crash (the next run on the same file replaces it).

## From upstream pull requests

Upstream's unmerged Linux build fixes (#40, #44) need no entry: `#undef Success` is already in `tools/screenshot.cpp:47`, and the fork builds on Linux. #37 (Print Screen hotkey registered as id 0) was merged upstream and is in the fork.

### 11. "Current monitor" capture: re-test the monitor under the cursor

- **Where:** `tools/screenshot.cpp:410-414`, `grabDesktop()` when `currentMonitor` is set.
- **What happens:** upstream PR #38 reported that this option captured the primary monitor instead of the one under the cursor. The current code picks the screen via `qApp->desktop()->screenNumber(QCursor::pos())`, so the original report looks fixed. That is inferred, not reproduced. `QDesktopWidget` is removed in Qt 6, so the Qt 6 port will rewrite this path anyway (`QGuiApplication::screenAt`).
- **Platform:** any multi-monitor setup. **Test:** by hand with the cursor on a non-primary monitor; after the port, a check that `screenAt` drives the choice.

### 12. Area selection on HiDPI and mixed-DPI screens

- **Where:** `dialogs/areadialog.cpp` (painting at `:488`, `resultRect()` at `:101-108`).
- **What happens:** upstream PR #35 (merged in 2016) fixed the area dialog showing the screenshot magnified by the device pixel ratio. The dialog has been reworked since, and `resultRect()` now scales the selection by the pixmap's single device pixel ratio. On mixed-DPI setups that combines with bug 4, and the selection may not match what ends up in the file. Inferred, needs reproduction.
- **Platform:** Windows with scaling above 100%, especially mixed. **Test:** by hand; select a known region and compare pixel sizes.

## Found while writing the Stage 3 plan

### 13. `updateHistory` never updates, it inserts

- **Where:** `tools/screenshotmanager.cpp:117-137`, `ScreenshotManager::updateHistory`.
- **What happens:** the lookup is `SELECT fileName FROM history WHERE URL IS NOT EMPTY AND fileName = ?`. `EMPTY` is not SQLite syntax, so `prepare` fails, `query.record().count()` is 0, and the function falls through to `saveHistory`, which inserts a second row for the file instead of filling in its URL. Visible as duplicate rows in the history dialog after uploading a screenshot that was already recorded.
- **Fix:** `WHERE fileName = ?` (and decide whether an existing URL should be overwritten), then test `exec()` and `next()` instead of `record().count()`.
- **Platform:** any. **Test:** headless, save a file then upload it, expect one row with the URL. Confirmed by the Stage 4 history test. **Status: fixed in Stage 5 (2026-10-14).** The lookup is `WHERE fileName = ?`, checked with `exec()` and `next()`; an existing URL is overwritten by the newer upload; an empty delete hash now stores an empty delete URL, as `saveHistory` does. `tst_history` asserts one row with the URL.

### 14. Uploads from the history dialog ignore the chosen service

- **Where:** `dialogs/historydialog.cpp:170`, `HistoryDialog::upload`.
- **What happens:** it reads `options/upload/service` (default `"imgur"`) and passes the raw value as the service name. Every other site reads `upload/service` as an index and maps it through `Uploader::serviceName`. So an upload started from the history dialog always goes to the default service, whatever the options say.
- **Fix:** read `upload/service` and map it through `Uploader::serviceName`.
- **Platform:** any. **Test:** headless with a stub network manager. Triage: fixed in plan step M12 (one line), not a Stage 5 session.

## Found by the Stage 4 screenshot naming tests

Recorded as current behaviour in `tests/tst_screenshot`; the test names below are the assertions a fix flips. Bugs 15 to 29 are triaged in `docs/modernization-plan.md`, section 7 (2026-10-14).

### 15. Collision naming can pick an existing file, which is then overwritten

- **Where:** `tools/screenshot.cpp`, `Screenshot::uniqueName` (called from `Screenshot::save`).
- **What happens:** for each file containing the name, it removes *every* occurrence of the name, `" ("` and `")"`, then parses what is left. With the name `2` and `2.png`, `2 (1).png`, `2 (2).png` on disk, `2 (2).png` reads as 0 (its `2` inside the suffix is removed too), the maximum is 1, and it returns `2 (2)`. `save()` does not check again and writes over `2 (2).png`. Any name containing digits can hit this (Numeric naming produces such names with `replace` off whenever a file already has the next number).
- **Fix:** parse only an exact `<name> (<n>)<extension>` pattern, or loop until the candidate does not exist.
- **Platform:** any. **Test:** `tst_screenshot::uniqueName_returnsExistingName` (confirmed).
- **Triage:** fix now (Stage 5 session C, with 17); priority: data loss.
- **Status:** fixed in Stage 5 (2026-10-14), with 17. `uniqueName` counts only files named exactly `<name> (<n>)<extension>` and then counts up until `<name> (<n>)<extension>` does not exist. Tests: `uniqueName_returnsExistingName` (now `2 (3)`), `uniqueName_skipsExistingCandidate`.

### 16. A failed save emits `finished` twice and still writes a history row

- **Where:** `tools/screenshot.cpp`, `Screenshot::save`.
- **What happens:** `if (!mOptions.result) emit finished();` fires for `Failure`, and the branch below then emits `finished` again (after `saveHistory` when file saving is on). `ScreenshotManager::finished` runs twice for one screenshot (`removeOne`, `activeCountChange` and `deleteLater` twice), and the history gets a row for a file that was never written. Same double `finished` when neither file nor clipboard is on.
- **Fix:** return after the failure `finished`, or emit it once at the end; skip `saveHistory` on failure.
- **Platform:** any. **Test:** `tst_screenshot::save_failure`, `tst_screenshot::save_neither` (confirmed).
- **Triage:** fix now (Stage 5 session D, with 18), after the bug 10 PR merges.
- **Status:** fixed in Stage 5 (2026-10-14). On `Failure`, `save()` emits `finished` once and returns: no history row, no optimize or upload hand-off. Left open: upload-only (file off and no clipboard image) still reports `Failure` and emits `finished` early, then again when the upload succeeds. That early `finished` currently ends the screenshot when an upload fails (the uploader emits no `done` on error), so it is left for the uploader work rather than removed here.

### 17. Collision suffix counts any file that contains the name (lower severity)

- **Where:** `Screenshot::uniqueName`.
- **What happens:** `file.contains(name)` matches longer names, so `screenshot1` next to `screenshot12.png` becomes `screenshot1 (3)`, and `shot (1) (1).png` becomes `shot (12)`. Odd numbers, no data loss on its own (see 15 for the case that loses data).
- **Fix:** the same pattern match as 15.
- **Platform:** any. **Test:** `tst_screenshot::uniqueName_prefixMatch`, `tst_screenshot::uniqueName_nestedSuffix`.
- **Triage:** fix now, with 15 (session C).
- **Status:** fixed in Stage 5 (2026-10-14), with 15: the exact pattern match ignores longer names and nested suffixes (`screenshot1 (1)`, `shot (1)`).

### 18. Temporary `.screenshot.` file kept when file and clipboard are both on (lower severity)

- **Where:** `Screenshot::save`, `Screenshot::unloadPixmap`.
- **What happens:** with the clipboard on, `save()` reloads the unloaded pixmap from the temp file and clears `mUnloaded`, so the file branch saves the pixmap again (a second encode) instead of renaming. The hidden `.screenshot.<n><ext>` file stays in the screenshot folder until the `Screenshot` is destroyed; a crash in between leaves it behind.
- **Fix:** keep the rename path when the pixmap was unloaded, reloading only for the clipboard.
- **Platform:** any. **Test:** `tst_screenshot::confirm_true_afterUnload_withClipboard`.
- **Triage:** fix now, with 16 (session D).
- **Status:** fixed in Stage 5 (2026-10-14). The clipboard gets a pixmap loaded from the temp file without clearing `mUnloaded`, so the file is renamed into place and no `.screenshot.` file is left.

## Found by the Stage 4 settings tests

### 19. WEBP (and any out-of-range index) is lost when the options dialog is saved

- **Where:** `dialogs/optionsdialog.cpp:180` loads `file/format` into `formatComboBox`, which lists only PNG, JPG and BMP; `:332` saves `currentIndex()` back. The same load/save pattern applies to `file/naming`, `options/previewPosition` and `upload/service`.
- **What happens:** `Screenshot::Format` has a fourth value, WEBP (3), which the main window accepts. `QComboBox::setCurrentIndex(3)` on a three-item box leaves no current item, so opening the options and saving them writes `file/format=-1`. The window then passes format `-1` to `Screenshot`, which is the out-of-range case of bug 2 (inferred from the code, not exercised).
- **Fix:** add WEBP to the combo box, and clamp or keep unknown indexes on load instead of saving `-1`.
- **Platform:** any. **Test:** `tst_settings::dialogOutOfRangeIndexes` asserts the current `-1`; it flips when this is fixed. Confirmed by the test.
- **Triage:** fix now (session E), unless the bug 2 PR already covers it.
- **Since the bug 2 fix:** the saved `-1` is read back as PNG, so there is no undefined behaviour any more, but a WEBP choice still silently becomes PNG. The dialog part stays open.
- **Status:** fixed in Stage 5 (2026-10-15). The format combo box lists WEBP, and `saveSettings` keeps the stored `file/format`, `file/naming` and `options/previewPosition` when the box has no current item instead of writing `-1`. `upload/service` still saves `-1`, left to M12. `tst_settings::dialogOutOfRangeIndexes` flipped.

### 20. PNG screenshots ignore the quality setting

- **Where:** `tools/screenshot.cpp:57-58`, the `Screenshot` constructor.
- **What happens:** for PNG it replaces `options/quality` with 80 before saving. For PNG, Qt maps the quality value to zlib compression (lossless), so the effect is a fixed compression level, not lower image quality. The options dialog greys out the quality slider for PNG, so this matches the UI; it is listed so the behaviour is a decision, not an accident.
- **Fix:** none needed unless PNG compression should become configurable; S05 (PNG optimization) is the natural place to decide.
- **Platform:** any. **Test:** `tst_settings::windowPngQualityIsForced` (Linux, under Xvfb). Confirmed by the test.
- **Triage:** no change; revisit only if S05 makes PNG compression configurable.

## Found while writing the Stage 4 uploader tests

### 21. `Uploader::upload` crashes on an unknown service name

- **Where:** `tools/uploader/uploader.cpp:93-128` (`Uploader::upload`); `ImageUploader::factory` (`tools/uploader/imageuploader.cpp:9-18`) returns `nullptr` for any name but `"imgur"` and `"pomf"`.
- **What happens:** `upload()` does not check the factory result: the `connect` calls warn, then `uploader->upload(fileName)` dereferences the null pointer and the app crashes. Nothing passes an unknown name today (`Uploader::serviceName` maps every index to `imgur` or `pomf`, and the history dialog's key from bug 14 is never written, so it gets the `"imgur"` default), but M12 and S03 rename the services and must keep `serviceName`, `factory` and the history dialog call in step.
- **Fix:** return early with `error` and `done(file, "", "")` when `factory` returns `nullptr`.
- **Platform:** any. **Test:** headless, `upload(file, "unknown")`; not in `tests/tst_uploader` today because it would crash the test. Confirmed from the code.
- **Triage:** deferred to M12 (factory falls back instead of returning `nullptr`); S03 keeps it. Both done-checks carry the test.

### 22. Cancelling an upload reports an error

- **Where:** `tools/uploader/pomfuploader.cpp:84` (cancel aborts the reply) and `:94-99` (the `finished` handler).
- **What happens:** `Uploader::cancel` makes each pomf uploader abort its reply. An aborted reply finishes with `OperationCanceledError` and an empty body, which the handler reports as `NetworkError` "Error reaching uploader" instead of `CancelError`. `Uploader` only suppresses `CancelError`, so every cancelled upload emits `error("Error reaching uploader")` then `done(file, "", "")`, and the main window shows an upload error after the user cancelled.
- **Fix:** in the pomf (or S03 custom) uploader, map `OperationCanceledError` to `CancelError`.
- **Platform:** any. **Test:** recorded by `tst_Uploader::cancel` (with a stub reply whose `abort()` behaves like Qt's HTTP reply); the fix flips that assertion. Not reproduced in the running app.
- **Triage:** deferred to S03 (the uploader rewrite); its done-check flips `tst_Uploader::cancel`.

## Found while writing the Stage 4 area dialog tests

### 23. Area selection loses a pixel at fractional scaling

- **Where:** `dialogs/areadialog.cpp:101-109`, `AreaDialog::resultRect`.
- **What happens:** the selection's left, top, width and height are each multiplied by the pixmap's device pixel ratio and truncated to `int`. At 150%, the selection (3, 5) 7x9 becomes (4, 7) 10x13 instead of covering device pixels 4.5 to 15 and 7.5 to 21, so the saved image can be a pixel narrower or shorter than what was selected, and the far edge is off by up to one pixel. Integer ratios (100%, 200%) are exact. Related to bug 12; S04 replaces this mapping.
- **Fix:** map the corners (`QRectF` scaled, then `toAlignedRect()` or rounded corners) instead of truncating origin and size separately.
- **Platform:** Windows (or X11 with `QT_SCALE_FACTOR`) at 125%, 150% or 175%. **Test:** `tst_AreaDialog::resultRect` rows "ratio 1.5 truncates" and "ratio 1.25 truncates" record the current values. Confirmed by the test; not reproduced in the running app.
- **Triage:** deferred to S04 (replaces the mapping); its done-check flips the fractional rows.

### 24. Typed area size ignores an upper-case X

- **Where:** `dialogs/areadialog.cpp:227-240` (`keyPressEvent`) and `:137-163` (`keyboardResize`).
- **What happens:** `keyPressEvent` lets the X key through whatever its text, so Shift+X or Caps Lock adds `X` to the typed size, but `keyboardResize` only splits on a lower-case `x`. Typing `100X100` and Enter shows the text on screen, then silently does nothing. The "only one x" check is case-sensitive too, so `1X2x3` can be typed.
- **Fix:** append a lower-case `x` for the X key (or split case-insensitively).
- **Platform:** any. **Test:** `tst_AreaDialog::typing_upperCaseX` and the "upper case X" row of `keyboardResize` record it. Confirmed by the test.
- **Triage:** fix now (session F).

## Found by the Stage 4 os helpers tests

### 25. "Run at startup" does nothing on Linux when `~/.config/autostart` is missing

- **Where:** `tools/os.cpp`, `os::setStartup` (Linux branch).
- **What happens:** it opens `~/.config/autostart/lightscreen.desktop` for writing without creating the directory or checking the result. On an account where the directory does not exist yet (common on a fresh install or a minimal desktop), the open fails, the write only logs `QIODevice::write ... device not open`, and the option is silently not applied. The file it does write also has no `Name=` key, which the Desktop Entry spec requires, and an unquoted `Exec` path, which breaks for a path with spaces.
- **Fix:** `QDir().mkpath` the autostart directory, check `open()`, add `Name=Lightscreen`, quote the executable path.
- **Platform:** Linux. **Test:** `tst_Os::startupWithoutAutostartDir` asserts that nothing is written (expected to change). Confirmed by the test.
- **Triage:** fix now (session G).
- **Fixed** (Stage 5): `setStartup` creates the directory, writes `Name=Lightscreen` and a quoted, escaped `Exec` (new `os::desktopExec`), and warns when the file cannot be written. Tests: `startupWithoutAutostartDir` flipped, new `startupUnwritableWarns` and `desktopExec`.

## Found while writing the Stage 4 hotkey widget tests

### 26. A hotkey cannot be set from the keyboard

- **Where:** `widgets/hotkeywidget.cpp:85-91` (`event`, `FocusIn`) and `:110-112` (`keyPressEvent`).
- **What happens:** when the hotkey button gets focus with Tab, it sets `mKeyboardFocus` and `keyPressEvent` then returns early for every key. Space and Enter never reach `QPushButton`, so the button cannot be pressed, and no other key is recorded: only a mouse click (any non-Tab focus) starts hotkey entry. Keyboard-only users cannot change a hotkey. The flag also stays set after focus leaves, until the next non-Tab focus.
- **Fix:** let Space and Enter through when `mKeyboardFocus` is set, and have them start hotkey entry (the same as a click: "Type your hotkey" and `grabKeyboard`).
- **Platform:** any. **Test:** `tst_HotkeyWidget::tabFocusIgnoresKeys` asserts that Space and Enter do nothing; the fix flips that assertion. Confirmed by the test; not tried in the running app.
- **Triage:** fix now (session H).

## Found by the Stage 4 main window tests

### 27. An upload error crashes the app when the tray icon is off

- **Where:** `lightscreenwindow.cpp:667-677` (`showUploaderError`) calls `notify(Screenshot::Failure)` unconditionally; `notify` (`:379-406`) calls `mTrayIcon->setIcon` without checking `mTrayIcon`, which is null when `options/tray` is off.
- **What happens:** with the tray icon disabled, any upload error (no pomf URL, unreadable file, network or host error, and every cancelled upload because of bug 22) dereferences the null tray icon and the app crashes. `cleanup` guards its own `notify` call with `options/tray && mTrayIcon`; `showUploaderError` does not.
- **Fix:** check `mTrayIcon` in `notify` (the window title part still applies), or guard the call as `cleanup` does.
- **Platform:** any. **Test:** confirmed under Xvfb by running `tst_mainwindow::uploadLastFlag` with `options/tray=false` (segfault in `QSystemTrayIcon::setIcon` from `notify`); the committed test turns the tray on so it can run, and can drop that line once this is fixed.
- **Triage:** fix now (session A); priority: crash.
- **Status:** fixed in Stage 5 (2026-10-14). `notify` sets the tray icon only when `mTrayIcon` exists; the window title (and the Windows taskbar overlay) still change. `tst_mainwindow::uploadErrorWithoutTray` runs an upload error with the tray off, and `uploadLastFlag` no longer turns the tray on.

### 28. A second screenshot request during the delay takes the first one's mode, then a timer loops forever

- **Where:** `lightscreenwindow.cpp:503-518` (`screenshotAction`, the `mLastMode` delay logic).
- **What happens:** every request waits at least 400 ms (plus `options/delay`), parked in the single field `mLastMode`. A second request while one is parked (two hotkeys in quick succession, or `lightscreen --screen --area`) takes the *parked* mode at once and clears `mLastMode`; its own mode is lost. When the first timer then fires it calls `screenshotAction(None)`, which parks `None` and re-arms itself every 400 ms for the life of the window. Each re-arm takes no screenshot but runs the pre-screenshot steps, so with "hide Lightscreen while taking a screenshot" on, the main window is hidden again right after the screenshot's cleanup showed it. `action(0)` starts the same loop. Later requests still work, handed over by the loop.
- **Fix:** queue requests (or capture `mode` in the timer's lambda) instead of sharing `mLastMode`, and ignore `None`. S01/S02 restructure this code.
- **Platform:** any. **Test:** `tst_mainwindow::twoScreenshotFlagsTakeTwoScreenshots` and `tst_mainwindow::actionOutOfRange` (Linux, under Xvfb).
- **Triage:** fix now (session B), after the bug 2 PR merges; priority: runaway timer.
- **Status:** fixed in Stage 5 (2026-10-14). Each request starts its own delay timer, which captures its mode (`takeScreenshot(mode)`); `mLastMode` is gone. `screenshotAction(None)`, and so `action(0)`, returns at once. A count of requests still waiting keeps "hide while taking a screenshot" in effect until the last of them is taken: `cleanup` restores the window only when none is pending. Not fixed here: a request arriving while the area selector of an earlier one is open is taken from inside the selector's event loop, so a whole-screen capture can include the selector overlay (left to S01/S02).

### 29. `lightscreen --quit` with no instance running starts a hidden instance

- **Where:** `main.cpp:83-84` runs `executeArguments` before `application.exec()`; `lightscreenwindow.cpp:361-362` handles `--quit` with `qApp->quit()`.
- **What happens:** when no instance is running, `--quit` calls `QCoreApplication::quit()` before the event loop exists. That only sets Qt's quit flag, which `exec()` resets when it starts, so the app keeps running. Because there was an argument, `main` does not show the window either: the result is a running, hidden Lightscreen (in the tray if the tray is on). With an instance running, SingleApplication forwards `--quit` and it works (the CI smoke). Separately, `--quit` bypasses `LightscreenWindow::quit`: no "are you sure" while screenshots are processing or uploading, no saved window position, no `finished` signal.
- **Fix:** have the primary instance exit when its own arguments contain `--quit` (for example queue the quit with `QTimer::singleShot(0, ...)`, or return from `main` early), and route `--quit` through `LightscreenWindow::quit` if the confirmation is wanted.
- **Platform:** any. **Test:** reproduced in the cloud container (Linux, Xvfb, portable `config.ini`): `timeout 8 xvfb-run -a ./lightscreen --quit` is still running when the timeout kills it. `tst_mainwindow::quitFlagQuitsTheApplication` records the bypass of `LightscreenWindow::quit` for an instance whose event loop runs.
- **Triage:** fix now (session I).
- **Status:** fixed in Stage 5 (2026-10-14). `executeArgument("--quit")` queues `QCoreApplication::quit` with `QTimer::singleShot(0, ...)`, so a quit requested before `exec()` runs once the event loop starts; a forwarded `--quit` still quits the running instance. Tests: `tst_mainwindow::quitFlagBeforeEventLoopQuits`, and the CI Linux smoke now also runs `./lightscreen --quit` with no instance and fails unless it exits with status 0 within ten seconds. Still open: `--quit` bypasses `LightscreenWindow::quit` (no confirmation, no saved position, no `finished`); `tst_mainwindow::quitFlagQuitsTheApplication` keeps recording that.

## Still to add

- Bugs the owner has hit on their own machine.
