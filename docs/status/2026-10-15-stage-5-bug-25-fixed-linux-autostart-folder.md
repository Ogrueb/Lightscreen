# 2026-10-15: Stage 5, bug 25 fixed (Linux autostart folder)

- `os::setStartup` (Linux) now creates `~/.config/autostart` when it is missing, writes `Type=Application`, `Name=Lightscreen` and an `Exec` line built by the new `os::desktopExec` (program in double quotes, with `"`, `` ` ``, `$`, `\` and `%` escaped per the Desktop Entry spec, then ` -h` when hidden), as UTF-8. If the directory or file cannot be written it logs `Could not write <path>` instead of failing silently. The Windows branch is unchanged.
- `tst_Os`: `startupWithoutAutostartDir` flipped (directory and file are created); the three content tests expect the new file; new `startupUnwritableWarns` and data-driven `desktopExec` (spaces, reserved characters, backslash, percent). They failed on master before the fix; all suites pass under Xvfb on Linux.
- Not changed: the path stays `~/.config/autostart` (not `$XDG_CONFIG_HOME`), and the file name stays `lightscreen.desktop` until M16's rename.
