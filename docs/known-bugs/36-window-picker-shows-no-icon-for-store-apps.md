# 36. The window picker shows no icon for Windows Store apps

- **Found:** by the owner in the Windows QA pass (A7, `2026-10-14-window-picker-icons.md` step 2), 2026-10-23: Settings has no icon; classic apps and browsers do.
- **Where:** `WindowPicker` (`tools/windowpicker.cpp`), the `GetClassLongPtr(GCLP_HICON/GCLP_HICONSM)` and `WM_GETICON` lookups.
- **What happens:** Store (UWP) apps such as Settings, Calculator and Photos run inside an `ApplicationFrameWindow` owned by `ApplicationFrameHost.exe`. That window has no class or window icon; the app's icon is in its package. So the picker shows none. Upstream behaves the same.
- **Possible fix (not done):** read the window's AppUserModelID (`SHGetPropertyStoreForWindow`, `PKEY_AppUserModel_ID`), open it in the Apps folder (`SHCreateItemInKnownFolder(FOLDERID_AppsFolder, ...)`) and ask the shell item for its image (`IShellItemImageFactory::GetImage`). About 40 lines.
- **Platform:** Windows 10/11. **Status:** known issue for 3.0 (owner, 2026-10-23); listed in `CHANGELOG.md`.
