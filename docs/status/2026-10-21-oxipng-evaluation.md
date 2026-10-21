# 2026-10-21: oxipng evaluation (docs only)

- Evaluated swapping `optipng` for `oxipng`. Outcome and numbers: `docs/decisions/2026-10-21-oxipng-replaces-optipng.md`. Recommendation: switch, inside S05, with a Linux fallback to `optipng` and a pinned, hash-checked `oxipng.exe` in the Windows zip (CI deploy step and M15).
- Found on the way: `optipng` is neither vendored nor bundled, so the Windows artifact has the optimize option disabled today ("optipng.exe not found"). Whichever optimizer is chosen, M15 (and the CI deploy step) must ship it for the option to work on Windows.
- No code changed. For S05: rename `tests/optipng_stub` to a neutral stub accepting `-out` and `--out`; update `tst_settings`' `findExecutable("optipng")` check; the plan's S05 text still says `optipng`.
