# 2026-10-21: optimizer choice recorded (docs only)

- The owner decided that optipng and oxipng are both supported as a user choice rather than swapping one for the other. Recorded in `docs/decisions/2026-10-21-optipng-and-oxipng-both-selectable.md`, which replaces the recommendation in `docs/decisions/2026-10-21-oxipng-replaces-optipng.md` (its measurements stay valid).
- For S05: new key `options/optimizer` (absent means automatic: oxipng if found, else optipng), a combo box beside the optimize checkbox, silent fallback to the other tool when the chosen one is missing, a stub built under both names, and tests for choice and fallback. For the CI deploy step and M15: the Windows zip ships both `oxipng.exe` and `optipng.exe`, pinned and hash-checked. The plan's S05 text still names only optipng.
- No code changed.
