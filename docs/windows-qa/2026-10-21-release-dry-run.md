# Release workflow dry run (M15)

Before tagging v3.0.0. A thread can start the run for you; otherwise: GitHub, Actions tab, "Release", "Run workflow" on `master`.

1. **Draft.** The run is green and the Releases page shows a draft pre-release "Dry run N (Lightscreen Rd 3.0, master)" with `lightscreen-rd-3.0-win64.zip` attached.
2. **Zip.** Download the zip and unzip it: it holds one folder `lightscreen-rd-3.0-win64` with `lightscreen.exe`, the Qt DLLs, `oxipng.exe`, `optipng.exe` and a `licenses` folder.
3. **Runs.** `lightscreen.exe` from that folder starts without a UAC prompt (SmartScreen may warn, since releases are unsigned), shows the tray icon and takes a whole-screen screenshot.
4. **Clean up.** Delete the draft (Releases page, the draft's "..." menu, Delete). Do not publish it.
