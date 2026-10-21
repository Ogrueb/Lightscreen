# Release workflow dry run (M15)

After the M15 PR is merged. A thread can start the run for you; otherwise: GitHub, Actions tab, "Release", "Run workflow" on `master`.

1. **Draft.** The run is green and the Releases page shows a draft pre-release "Dry run N (Lightscreen 2.5, master)" with `lightscreen-2.5-win64.zip` attached.
2. **Zip.** Download the zip and unzip it: it holds one folder `lightscreen-2.5-win64` with `lightscreen.exe` and the Qt DLLs.
3. **Runs.** `lightscreen.exe` from that folder starts without a UAC prompt (SmartScreen may warn, since releases are unsigned), shows the tray icon and takes a whole-screen screenshot.
4. **Clean up.** Delete the draft (Releases page, the draft's "..." menu, Delete). Do not publish it.
