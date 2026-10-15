# 25. "Run at startup" does nothing on Linux when `~/.config/autostart` is missing

- **Found:** found by the Stage 4 os helpers tests.
- **Where:** `tools/os.cpp`, `os::setStartup` (Linux branch).
- **What happens:** it opens `~/.config/autostart/lightscreen.desktop` for writing without creating the directory or checking the result. On an account where the directory does not exist yet (common on a fresh install or a minimal desktop), the open fails, the write only logs `QIODevice::write ... device not open`, and the option is silently not applied. The file it does write also has no `Name=` key, which the Desktop Entry spec requires, and an unquoted `Exec` path, which breaks for a path with spaces.
- **Fix:** `QDir().mkpath` the autostart directory, check `open()`, add `Name=Lightscreen`, quote the executable path.
- **Platform:** Linux. **Test:** `tst_Os::startupWithoutAutostartDir` asserts that nothing is written (expected to change). Confirmed by the test.
- **Triage:** fix now (session G).
- **Fixed** (Stage 5): `setStartup` creates the directory, writes `Name=Lightscreen` and a quoted, escaped `Exec` (new `os::desktopExec`), and warns when the file cannot be written. Tests: `startupWithoutAutostartDir` flipped, new `startupUnwritableWarns` and `desktopExec`.
