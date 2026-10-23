# 31. In Windows dark mode the main window uses outdated inverted icons

- **Found:** by the owner during the Windows QA pass (2026-10-23), CI build from `master` at `06db33a`, Windows 11 in dark mode.
- **Where:** `os::icon` (`tools/os.cpp`), the `inv/` icon set (`images/inv/`, `lightscreen.qrc`), the main window's buttons (`LightscreenWindow`).
- **What happens:** the main window follows Windows dark mode: dark buttons with light icons. Upstream 2.5 shows light buttons with dark icons. Two of the light icons are wrong:
  - **Upload** shows the old Imgur logo, a green and black "i". `images/inv/upload.png` was `inv/imgur.png` before M12 renamed it. Upstream redrew only the normal set's icon as an upload arrow (commit `4140f8e`, "New icons"), never the inverted one.
  - **Options** shows a wrench (`images/inv/configure.png`); the normal set has the list icon.
- **Why it shows now:** `os::icon` picks `:/icons/inv/<name>` when the palette's button colour is dark (`value() <= 125`). Qt 5 ignored Windows dark mode, so upstream never reached the `inv/` set on Windows, only on dark Linux themes. Qt 6.8's `windows11` style, the default on Windows 11, follows dark mode, so the set now appears.
- **Fix direction (owner's choice):**
  1. **Keep the dark look** and bring the `inv/` set in line with the normal set: an inverted upload arrow and list icon, and a check of the other `inv/` icons.
  2. **Keep the upstream look:** force a light palette, or a style that ignores dark mode (`windowsvista` or `fusion` with a light palette), so the normal icons are used as before.
- **Platform:** Windows 10/11 in dark mode; Linux with a dark theme (an upstream issue there too). **Test:** by hand; add a check to `docs/windows-qa/` with the fix.
- **Status:** fixed (2026-10-23). The owner chose to keep following the Windows theme, with a generic upload icon. `images/inv/upload.png` and `images/inv/configure.png` are now the normal set's upload arrow and list icon, light on dark: the greys flipped (35 becomes 220, as in the rest of the `inv/` set) and the transparency kept. The other 19 `inv/` icons already matched. New test `tst_Os::invertedIconsMatchTheNormalSet`: every inverted icon has the same size and transparency mask as its normal one. It fails on the old icons and passes on the new ones. Windows check: `docs/windows-qa/2026-10-23-dark-mode-icons.md`.
