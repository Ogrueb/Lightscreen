# 38. Quitting during an optimization left the tool running and a `.optimize.tmp` file behind

- **Found:** by the owner in the Windows QA pass (A11, `2026-10-22-png-optimizers.md` step 5), 2026-10-23, Windows 11: after a burst of screenshots and an immediate quit, one `.optimize.tmp` file was left in the screenshot folder. The screenshots themselves were intact, and the next start optimized the pending one and removed the file.
- **Cause:** `Optimizer`'s destructor kills the running tool and removes its temporary file, but its owner, the `ScreenshotManager` singleton, is never destroyed. At quit the tool went on running on its own and wrote its output file, which nothing renamed. Introduced with the queue in S05.
- **Fix:** `Optimizer::stop()` (also called by the destructor) kills the tool and removes the temporary file; `ScreenshotManager` calls it on `QCoreApplication::aboutToQuit`. The file stays in the pending list and is optimized after the next start, as before.
- **Test:** `tst_Optimizer::quit_stopsTheRunningTool` runs the application's event loop and exits it while a slow stub tool runs: no temporary file, the original unchanged, the file still pending. It fails without the fix (the tool finished and replaced the file).
- **Platform:** all. **Status:** fixed, awaiting the owner's re-check (`2026-10-22-png-optimizers.md`).
