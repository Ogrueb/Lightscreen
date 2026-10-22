# First run with no settings (S06 defaults, Print Screen on Windows 11)

Without touching your real settings: unzip the `lightscreen-win64` artifact of the latest `master` CI run into a new folder and put an **empty** `config.ini` next to `lightscreen.exe` (portable mode, so everything is stored in that file).

1. **First-run options.** Start `lightscreen.exe`: the Options dialog opens first, with only an OK button. It shows format PNG, prefix `screenshot.`, folder `Screenshots` (relative to the exe in portable mode), the tray icon on, and on the Hotkeys tab "Fullscreen" enabled on Print Screen with the other captures off. Press OK.
2. **Defaults in use.** Take a whole-screen screenshot from the tray menu: `Screenshots\screenshot.1.png` appears next to the exe.
3. **Print Screen on Windows 11.** Press Print Screen with Windows' "Use the Print screen key to open screen capture" on (Settings > Accessibility > Keyboard; on by default in Windows 11). Note what happens: Lightscreen saves a screenshot, Windows' Snipping Tool opens instead, or Lightscreen shows a hotkey error. Then turn that Windows setting off, restart Lightscreen and press Print Screen again: Lightscreen saves the screenshot. If Snipping Tool wins in the first case, that becomes a known issue (and a note in the release notes), not a blocker.
4. Delete the test folder afterwards.
