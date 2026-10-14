# Status

What is done, what changed from the plan, and what is open. Newest first.

## 2026-10-14: Stage 4 session 7, main window plumbing tests

- `tests/tst_mainwindow` (16 test functions, 33 rows with data): every `executeArgument` flag reaches its slot (`--wake` shows the window; `--screen`, `--area`, `--activewindow`, `--pickwindow` reach `ScreenshotManager::take` with their mode; `--folder` opens `file/target`; `--uploadlast` uploads the `lastScreenshot` setting; `--viewhistory` and `--options` open their dialogs; `--quit` quits the event loop); the "one argument that is the program means `--wake`" branch; `connectHotkeys` ids 1 to 6 in the order screen, window, area, windowPicker, open, directory, with disabled actions keeping their ids; ids 1 to 6 reaching `action()`. No production code changed.
- Runs on Linux under Xvfb only; the whole test skips on Windows (the `os::setStartup` registry write) and when the platform is not xcb. `HOME` points at a temporary directory so the autostart file is never touched. How the test observes things without seams: `docs/decisions.md`, Stage 4 main window entry.
- Bugs found (new in `docs/known-bugs.md`): **27**, an upload error crashes the app when the tray icon is off (null `mTrayIcon` in `notify`; reproduced, segfault); **28**, a second screenshot request within the 400 ms delay takes the first request's mode, loses its own, and leaves a timer re-arming itself forever (which re-hides the window when "hide while taking a screenshot" is on); **29**, `lightscreen --quit` with no instance running leaves a hidden instance running (reproduced with the built app under Xvfb).
- Other current behaviour the tests record: `-h`, the flag the autostart entry adds for "start hidden", is handled by nothing; the window stays hidden only because `main` shows it when there are no arguments. The `--wake` branch matches any single argument that *contains* the executable's file name. Flags are case-sensitive and unknown ones are ignored silently. `--quit` skips `LightscreenWindow::quit` (no confirmation while processing or uploading, no saved position, no `finished`). `action(7)` warns; `action(0)` is taken as a screenshot mode (bug 28's loop). An enabled action with an empty hotkey shows the "could not be registered" box naming the action's settings name (`area`), not a key, and "Disable" turns it off; on Linux `UGlobalHotkeys::registerHotkey` reports success even for a key it cannot map, so only an empty hotkey reaches that box.
- Verified in the cloud container (Ubuntu 24.04, Qt 5.15.13): all pass under `xvfb-run -a`; skipped with `QT_QPA_PLATFORM=offscreen`. Windows only through CI (compiles, then skips).
- This was the last Stage 4 session to land, so it also updated the test list in `docs/build.md`, "Tests" (open item from the settings entry below). With all eight sessions merged, Stage 4 is complete.

## 2026-10-14: Stage 4 session 5, hotkey widget tests

- `tests/tst_hotkeywidget` (37 cases) covers `widgets/hotkeywidget` as plan section 6 item 5 describes: `setHotkey`/`hotkey` round trip for the six `OptionsDialog` defaults (incl. `Print`, `Ctrl+Print`, `Ctrl+Alt+Print`, `Ctrl+PgUp`), key presses (the `event->key() + modifiers` line M09 rewrites), modifier-only keys, `Escape` and other invalid keys, mouse vs Tab focus, and `LanguageChange`. Runs under Xvfb (the focus tests activate the window by hand, since Xvfb has no window manager) and offscreen. No production code changed.
- Current behaviour the tests record (not fixed):
  - With focus (the real case), `Escape` gives "Invalid hotkey" only for an instant: the widget moves focus to its parent, and the `FocusOut` handler emits `invalidHotkeyError`, turns the text red for a second and puts the old hotkey's text back. So the user sees the old hotkey in red, never the words "Invalid hotkey". Nothing connects `invalidHotkeyError`. Without focus the "Invalid hotkey" text stays.
  - Tab focus makes the widget ignore every key, Space and Enter included, so a hotkey can only be set with the mouse. Now bug 26.
  - The keypad flag is stripped (`Ctrl`+keypad 5 saves as `Ctrl+5`); the context-menu key counts as a modifier; `setHotkey("")` leaves an empty button (the "Click to select hotkey..." prompt is gone); a two-key sequence (`Ctrl+A, B`) survives `setHotkey`/`hotkey` although typing can only produce one key; an unknown name gives an empty hotkey.
- Not tested, found while reading: `HotkeyWidget` without a parent crashes in `setHotkey` (`parentWidget()->setFocus()` unchecked). Every instance lives in the options dialog, so it cannot happen today. In `event()`, the `KeyPress` part of the last `else if` is dead code, since an earlier branch handles `KeyPress`.
- `QTest::keyClick` asserts on keys without an ASCII value (AltGr, Super, `Key_unknown`); the test sends those with `QTest::sendKeyEvent` and an empty text.
- Verified in the cloud container (Ubuntu 24.04, Qt 5.15.13) under `xvfb-run -a` and with `QT_QPA_PLATFORM=offscreen`, five runs each. Windows through CI only; Print Screen on a real Windows keyboard (which may deliver only a key release) is not covered and stays on M09's Windows QA list.

## 2026-10-14: Stage 4 session 8, os helpers tests

- `tests/tst_os` (11 test functions, offscreen-safe) covers `tools/os` as plan section 6 item 8 describes: `getDocumentsPath` (non-empty and absolute everywhere; on Linux `$HOME/Documents`, not created), `icon()` choosing `:/icons/` or `:/icons/inv/` by the HSV value of the given colour or, without one, of the palette's `Button` colour, and on Linux `setStartup` with `HOME` pointed at a temporary directory. `setStartup` is not called on Windows, where it writes the real `HKCU\...\Run` key; those tests are compiled out there. No production code changed.
- Recorded behaviour: the icon threshold is `value() > 125` (125 gives the inverted set, 126 the normal one, so pure blue counts as light); only `QPalette::Button` matters; the four `lightscreen.*` icons have no inverted variant, so on a dark button they come back with no pixmap (nothing asks for them through `icon()` today); an unknown name gives an empty icon silently. The `.desktop` file is exactly `[Desktop Entry]\nExec=<path>[ -h]\nType=Application` with no trailing newline, no `Name=` key (required by the Desktop Entry spec) and the path unquoted, so a path with spaces would break it; an existing file is replaced; `setStartup(false)` removes it and leaves the directory.
- **New bug 25:** when `~/.config/autostart` does not exist, `setStartup(true)` writes nothing and only logs a Qt warning, so "run at startup" silently does nothing on a fresh Linux account. Asserted as `expected to change (bug 25)` in `tst_Os::startupWithoutAutostartDir`.
- Verified in the cloud container (Ubuntu 24.04, Qt 5.15.13): all pass under `xvfb-run -a make check` and with `QT_QPA_PLATFORM=offscreen`. Windows through CI.

## 2026-10-14: Stage 4 session 6, area dialog geometry tests

- `tests/tst_areadialog` (59 cases, offscreen-safe; the dialog is never shown): `resultRect` at pixmap ratios 1, 2, 1.5 and 1.25, `keyboardResize` parsing (direct and typed through `keyPressEvent`), `limitPointToRect`, and the eight handle rectangles after `updateHandles`. A test subclass opens the protected members; the screenshot's pixmap is set through `pixmap()` to skip the confirmation path. No production code changed.
- Current behaviour the tests record (not fixed): `resultRect` truncates each of left, top, width and height after scaling, so fractional ratios lose up to a pixel (now bug 23), and it does not normalize the selection; Shift+X adds an `X` that `keyboardResize` ignores (now bug 24); a typed size with no selection is placed at the dialog's top left; a size that would leave the dialog is dropped silently, while one that reaches the edges exactly is accepted; each typed digit is checked against the dialog's larger side (the whole number is not); handles sit on the selection shrunk by one pixel at the right and bottom, and for a selection smaller than 10 pixels the right and bottom handles land left of and above it; `limitPointToRect` clamps to the inclusive right/bottom edge and does not normalize an inverted rectangle.
- Verified in the cloud container (Ubuntu 24.04, Qt 5.15.13): all test suites pass under `xvfb-run -a make check`, and `tst_areadialog` with `QT_QPA_PLATFORM=offscreen`. Windows through CI.

## 2026-10-14: Stage 4 session 4, uploader core tests

- `tests/tst_uploader` (32 cases, offscreen-safe): `serviceName`, `factory`, `ImageUploader` settings round trip (static and per-instance), `Uploader::upload` to `done`, progress over two uploads, `cancel`, the error path, and the pomf request (`files[]` field, multipart body, `<pomf_url>/upload.php`) and response parsing as S03's baseline. Network is a `QNetworkAccessManager` subclass returning `CannedReply`; its `abort()` mirrors Qt's HTTP reply (error `OperationCanceledError`, `finished` emitted at once). No production code changed.
- Current behaviour the tests record (not fixed): cancelling (now bug 22 in `docs/known-bugs.md`) an upload reports `error("Error reaching uploader")` then `done(file, "", "")` for each running upload, because pomf turns the abort into `NetworkError`, not `CancelError`; one failed upload resets the total progress to 0 while others still run; the final reset to 0 after the last upload is not signalled; pomf ignores a network/HTTP error when the body is a JSON object; `{"success": true, "files": []}` is `done` with an empty URL and no error; `"success": "true"` (a string) is a host error; INI settings come back as strings after a reload (`42` becomes `"42"`).
- Not tested, found while reading (now bug 21): `Uploader::upload` with a service name `factory` does not know (anything but `imgur`/`pomf`) dereferences a null pointer and crashes. Nothing passes such a name today (the history dialog reads `options/upload/service`, which nothing writes, so it always gets the `imgur` default: bug 14), but M12 and S03 change the names and must keep `serviceName`, `factory` and that call in step.

## 2026-10-14: Stage 4 safety net, settings round-trip

- `tests/tst_settings` (plan section 6, item 1). It records what `OptionsDialog` writes on empty settings (`defaults.golden`, 53 keys), what survives a save and reload with every widget changed (`roundtrip.golden`), and what `LightscreenWindow::screenshotAction` reads, both from near-empty settings and from the dialog's defaults. Tests record current behaviour; nothing was fixed. No production code changed.
- Behaviour the tests recorded (each is asserted, so a change to it fails a test):
  - **WEBP cannot round-trip through the dialog.** The format combo box has PNG, JPG and BMP only, so `file/format=3` is saved back as `-1` after the options dialog is opened and saved. The same happens to any out-of-range naming, preview position or upload service index. The window would then pass format `-1` to `Screenshot`, which is bug 2's out-of-range case (inferred from the code, not exercised here). Asserted by `dialogOutOfRangeIndexes`.
  - **On Linux, `options/cursor` and `options/playSound` are always saved as `false`,** and `options/optimize` too when `optipng` is not on `PATH`: `loadSettings` forces them off. Opening the dialog also starts `optipng` once (its availability check is `QProcess::startDetached`).
  - **`loadSettings` writes settings:** it clears `upload/imgur/access_token`, `refresh_token`, `account_username` and `expires_in` whenever no Imgur account is stored. Nothing in `saveSettings` persists an Imgur account set on the widget.
  - **The window's defaults differ from the dialog's** for a missing key: file saving off (dialog: on), empty prefix (dialog: `screenshot.`), the current directory as target (dialog: `Documents/Screenshots`), cursor on (dialog: off on Linux). The window's format default (0) is unreachable, because without `file/format` it opens the options dialog instead. `options/animations` is read but never written.
  - **`screenshotAction` caches the options per window** until `applySettings` runs; a setting changed in between is not seen. PNG screenshots get quality 80 whatever `options/quality` says (the `Screenshot` constructor; asserted by `windowPngQualityIsForced`).
- Windows CI trap for every test: with Qt 5.15.2 and the MSVC on `windows-latest` (14.51), `QList::operator==`/`!=` (so comparing two `QStringList`s, including `QCOMPARE` on them) does not compile: Qt's `qlist.h` calls `stdext::make_checked_array_iterator`, which that MSVC removed. `tst_settings` compares with `std::equal` instead.
- Added to `docs/known-bugs.md` as bugs 19 (WEBP saved as `-1`) and 20 (PNG quality forced to 80), each naming its test.
- The four `LightscreenWindow` tests need X: they run on Linux under `xvfb-run` and are skipped offscreen and on Windows (why: `docs/decisions.md`, Stage 4 settings entry). The dialog tests run everywhere.
- Verified in the cloud container (Ubuntu 24.04, Qt 5.15.13): all 9 functions pass under `xvfb-run -a make check`; with `QT_QPA_PLATFORM=offscreen` 5 pass and the 4 window tests skip. Windows only through CI.
- `docs/build.md` still says `tst_smoke` is the only test; left for whoever lands the last Stage 4 session, to avoid four-way conflicts.

## 2026-10-14: Stage 4 safety net, screenshot naming and saving

- `tests/tst_screenshot` (39 test functions) records what `tools/screenshot` does today: `getName` for all four namings, `extensionFor`, `uniqueName`, `save()` (file, clipboard, both, neither, failure, `replace`, every format, quality), the `.screenshot.` temp file and `confirm`. Passes under Xvfb and offscreen on Linux (Qt 5.15.13); Windows through CI.
- Bug 2 is a skipped test with the intended assertion; the Stage 5 fix deletes the `QSKIP` (`docs/decisions.md`).
- Quirks the tests pin down, none fixed: `uniqueName("2")` with `2.png`, `2 (1).png` and `2 (2).png` present returns `2 (2)`, an existing file, which `save()` then overwrites; `uniqueName` counts any file containing the name (`screenshot1` next to `screenshot12.png` gives `screenshot1 (3)`) and joins nested suffixes (`shot (1) (1).png` gives `shot (12)`); Numeric naming counts every extension and flipped and unflipped names share one counter; a `%1` in the prefix is substituted too. `save()` emits `finished` twice when it fails or when neither file nor clipboard is on, and a failed file save still writes a history row. With file and clipboard both on, the unloaded temp file is re-encoded instead of renamed and stays on disk until the `Screenshot` is destroyed. PNG quality is always forced to 80. The overwrite, the double `finished`, the prefix matching and the leftover temp file are now bugs 15-18 in `docs/known-bugs.md`, not yet triaged.
- No production code changed.

## 2026-10-14: Stage 4 safety net, history tests

- `tests/tst_history/` (19 test functions) covers `tools/screenshotmanager`'s history as plan section 6 item 3 describes: `initHistory` (directory, file, the four columns), `saveHistory` with history on (the default) and off, `updateHistory` for an existing file, a new file, an empty URL and history off, `Uploader::done` reaching `updateHistory`, `removeHistory`, `clearHistory`, the seam's history path, and portable mode. Tests only; no production code changed.
- **Bug 13 confirmed.** `updateHistory` on a file that already has a row inserts a second row and leaves the first without its URL. Cause as described in `docs/known-bugs.md`: SQLite rejects the lookup with `no such column: EMPTY`, so the function always falls through to `saveHistory`. Three assertions are marked `expected to change (bug 13)` for the Stage 5 fix to flip.
- Also recorded: the delete hash is stored as `https://imgur.com/delete/<hash>` (M12 changes this); saving the same file twice gives two rows; with history off `saveHistory` does not even create the database; `removeHistory` and `clearHistory` ignore the history setting.
- Portable mode is tested through the default constructor: the test writes a `config.ini` next to its own executable, checks portable mode, settings file and history location, and removes both files. Both tests skip if such files already exist there.
- Verified in the cloud container (Ubuntu 24.04, Qt 5.15.13) under `xvfb-run -a make check` and with `QT_QPA_PLATFORM=offscreen`.

## 2026-10-14: M02 test harness and seams

- `lightscreen.pro` is split: the sources, Qt modules, compile flags, platform libraries and the two vendored `.pri` files are in `src.pri`; `lightscreen.pro` keeps the target, `main.cpp`, the icon, the manifest and the linker subsystem flags. `tests/tests.pro` (subdirs) and `tests/common.pri` build tests against the same `src.pri`. One placeholder test, `tests/tst_smoke`. How to build and run: `docs/build.md`, "Tests".
- Seams, as the plan specifies: `ScreenshotManager(QSettings *, historyPath, parent)` + `setInstance`; `Uploader::setInstance` + `setNetwork`; `Screenshot::extensionFor` + `uniqueName` (the collision loop from `save()` moved verbatim, quirks kept). Existing call sites are unchanged; no behaviour change intended.
- Verified in the cloud container (Ubuntu 24.04, Qt 5.15.13 from apt), on a fresh clone of the branch: the app builds with the same warnings as `master`, and it starts and stops with `--quit` under Xvfb (the M01 smoke script); the tests build and `tst_smoke` passes under `xvfb-run -a make check` and with `QT_QPA_PLATFORM=offscreen`.
- CI (M01's workflow) now builds the tests and runs `make check` in both jobs: Linux under `xvfb-run -a`, Windows with `QT_QPA_PLATFORM=offscreen`. Windows is verified only through CI; nobody ran it on a Windows machine.
- Stage 4 can start once this is merged.

## 2026-10-14: M01 GitHub Actions CI

- `.github/workflows/ci.yml`: two jobs on every pull request and push to `master`. `linux` (ubuntu-latest, Qt 5.15 from apt) builds and runs the Linux smoke (start under Xvfb in portable mode, stop with `--quit`, expect exit 0 within ten seconds). `windows` (windows-latest, Qt 5.15.2 msvc2019_64, MSVC x64) builds, runs `windeployqt` and uploads `lightscreen-win64` as an artifact.
- README: the upstream AppVeyor and Coverity badges are replaced by the fork's CI badge (the rest of the README is M14's).
- `docs/build.md`: Linux section now verified, plus a "Continuous integration" section. Details the plan left open are in `docs/decisions.md` (M01 entry).
- Verified in the cloud container: the Linux build and the smoke script pass locally (Ubuntu 24.04, Qt 5.15.13). Both jobs green on PR #7; the Windows job produced the `lightscreen-win64` artifact (about 42 MB).
- Still open from the done-check: the optional one-time Windows QA (download the artifact and start it on the owner's machine).
- M02 can build on this: its `make check` steps go into the same two jobs.

## 2026-10-14: Stage 3 plan written

- `docs/modernization-plan.md`: 16 mechanical steps (M01-M16) and 7 structural steps (S01-S07, two optional), each one PR with dependencies, files, a done-check and a merge owner; an overall merge order; Stage 4 test priorities with the seams they need; Stage 5 bug triage. Decisions and rejected alternatives are in `docs/decisions.md` (Stage 3 entry).
- Two plan steps come **before Stage 4**: M01 (GitHub Actions CI on ubuntu and windows-latest) and M02 (test harness, three small seams). Stage 4 cannot start until both are merged.
- Stage 5 sessions: bugs 1, 2, 5 (bounds only), 10 (local defects only) and the new bug 13. Everything else is deferred to a named plan step.
- Two new bugs found while reading the code are in `docs/known-bugs.md`: 13 (`updateHistory` inserts instead of updating; inferred) and 14 (history-dialog upload ignores the chosen service; confirmed).
- Owner decisions taken in the Stage 3 thread (plan section 8): the Windows jump list is dropped in the port, with S07 as an optional later re-addition; the `K/Lightscreen` settings identity is kept at the rename. The fork's name is still needed before M16.
- PR #4 (`docs/upstream-prs.md`, bugs 11 and 12) was already merged when this stage started; the plan is written against `master` at `d92b3ca`.
- Nothing was built in this thread: the cloud container has no Qt installed. The plan's API claims come from the code and the Qt documentation. M01 is where a build first runs in CI.

## 2026-10-14: Upstream PRs recorded

- The upstream issue tracker (`ckaiser/Lightscreen`) is disabled, so there are no upstream issues to draw on. The 10 upstream PRs are summarized in `docs/upstream-prs.md`. The Linux `Success` clash (#40) and the active-monitor capture bug (#38) are already fixed in the code. Multi-monitor and HiDPI capture (#38, #35) are worth re-testing.
- Folded the upstream PR findings into `docs/known-bugs.md` (bugs 11 and 12, plus a note on bug 4). #40 and #44 need no entry, since the Linux build already works.

## 2026-10-14: goals.md and known-bugs.md drafted

- `docs/goals.md`: purpose, the six-point definition of "resilient", platform and compatibility commitments, out-of-scope list. Decisions taken by the owner in the drafting thread: Qt 6, Windows 10 minimum, Linux X11 kept as a CI-tested target, update check moved to GitHub Releases. Recorded in `docs/decisions.md`. Only the fork's name is still open, and it is not needed before Stage 3.
- `docs/known-bugs.md`: ten entries. Three confirmed from the code (window-picker icon handle, `extension()` missing return, missing Linux error sound), five inferred from the code and awaiting reproduction (mixed-DPI desktop capture, window capture margins, silent update failure, dead SSL workaround, dead macOS block), two reported by the owner (HDR browser brightness, PNG optimization queue).
- Upstream's issue tracker is disabled, so there was no issue list to mine. `docs/upstream-prs.md` (being written by another thread) may add entries from the five unmerged upstream PRs; fold those in when it lands.
- Stage 3 can start once this PR is merged.

## 2026-10-14: Stage 2 inventory

- `docs/inventory.md`: dependencies, module map, deprecated and Qt 6-removed APIs, test coverage, build/CI/packaging. Survey only; no code changed.
- Correction to `docs/build.md`: `Screenshot::extension()` (C4715) does handle WEBP inside its `switch`. The missing return is reached only when the format read from settings is outside 0-3. See `docs/inventory.md`, "Other compiler-flagged code".
- Not checked from the Stage 2 session (outbound access limited), marked *(not verified here)* in the inventory: whether `lightscreen.com.ar` is still up (the updater and the pomf host list use it), and the end date of Qt 5.15 support.
- Still open: CI for Windows and Linux. (`docs/goals.md` and `docs/known-bugs.md`: see the entry above.)

## 2026-10-14: Submodules vendored

- SingleApplication and UGlobalHotkey are now plain files in the repo instead of submodules (see `docs/decisions.md`). A plain `git clone` is enough to build; the `git submodule update --init` step is gone from `docs/build.md`.
- Linux build check, done in the cloud container (Ubuntu 24.04, Qt 5.15.13 from apt): `qmake` and `make` succeed with the vendored sources, and the binary starts and keeps running under Xvfb. Two findings for whoever owns the Linux build:
  - `qtbase5-private-dev` is needed in addition to the packages listed in `docs/build.md`, because UGlobalHotkey includes `qpa/qplatformnativeinterface.h`.
  - With `QT_QPA_PLATFORM=offscreen` (no X server) the app segfaults at startup; it needs a real or virtual X display.
- Windows build verified by the owner on a fresh clone (VS 2022, Qt 5.15.2): builds, launches, same 14 warnings as Stage 1. Existing checkouts should run `git submodule deinit --all` (or reclone) after pulling this change.

## 2026-10-14: Stage 1 done

- Windows x64 release build works on VS 2022 + Qt 5.15.2 with no source changes; see `docs/build.md`.
- Open from Stage 1: Linux build verification, CI for Windows and Linux, `docs/goals.md` and `docs/known-bugs.md` (by the owner).
