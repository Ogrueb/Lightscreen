# 37. The shutter sound plays together with Windows' notification sound

- **Found:** by the owner in the Windows QA pass (A9, `2026-10-15-sound-cues.md` step 1), 2026-10-23, Windows 11: with "Sound cue" on, a screenshot plays the shutter and Windows' notification sound.
- **Cause (confirmed by the owner):** the second sound belongs to the "Saved to …" tray pop-up (Options tab > Interface > "Tray icon Popup", on by default), for which Windows plays its notification sound. Turning the pop-up off leaves only the shutter. Qt's `QSystemTrayIcon::showMessage` has no way to ask for a silent pop-up (`NIIF_NOSOUND`). Upstream 2.5 shows the same kind of pop-up and very likely does the same (not tested).
- **Not fixed (owner, 2026-10-23):** documented instead. A fix would mean calling `Shell_NotifyIcon` directly with internal identifiers of Qt's tray icon, which Qt does not expose.
- **Workaround:** turn off "Tray icon Popup" in Lightscreen Rd, or turn off "Play a sound when a notification arrives" for Lightscreen Rd in Windows (Settings > System > Notifications).
- **Platform:** Windows 10/11. **Status:** known issue for 3.0, listed in `CHANGELOG.md` and the release notes.
