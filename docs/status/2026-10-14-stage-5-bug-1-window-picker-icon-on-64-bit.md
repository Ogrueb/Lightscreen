# 2026-10-14: Stage 5, bug 1 (window picker icon on 64-bit Windows)

- `tools/windowpicker.cpp`: the icon is now read with `GetClassLongPtr(GCLP_HICON)`, falling back to `GCLP_HICONSM`, then to `WM_GETICON` with `ICON_BIG` (sent with `SendMessageTimeout`, 100 ms, abort if hung; why: `docs/decisions.md`). Windows-only code; Linux unchanged.
- Verified: Windows build on CI and the `lightscreen-win64` artifact. Not verifiable headless; needs the owner's Windows QA (hover a few windows in the picker, including a modern app such as Settings or a browser, and see their icons).
