# CMake build on Windows (PR #44, M11)

The plan's one-time check that the build instructions work on the owner's machine. Steps 1 and 2 are the build; 3 and 4 use the result (or the `lightscreen-win64` artifact of the latest `master` CI run).

1. **Build.** In the checkout, delete any old `build\` folder, then from a plain `cmd` prompt run the commands under "Windows > Build" in `docs/build.md` (`vcvars64.bat`, Qt on `PATH`, `cmake --preset windows-msvc-release`, `cmake --build --preset windows-msvc-release`). Both finish without errors and `build\windows-msvc-release\lightscreen.exe` exists. If `cmake` is not found, see the note in "One-time setup".
2. **Tests.** `ctest --preset windows-msvc-release` in the same prompt: 100% tests passed.
3. **Exe properties.** In Explorer, `lightscreen.exe` shows the Lightscreen icon; Properties > Details shows file and product version 3.0.0.0, description "Lightscreen Rd Screenshot Tool", product "Lightscreen Rd", and the copyright line naming Christian Kaiser and the Lightscreen Rd contributors.
4. **Runs.** After `windeployqt` (see "Run / package"), `deploy\lightscreen.exe` starts without a UAC prompt, shows the tray icon and takes a whole-screen screenshot.
