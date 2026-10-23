# Area capture after S01 (dialog moved out of `tools/screenshot`)

Steps 1 to 6 (autoclose off and on, magnifier, keyboard size, cancel, refresh) passed on 2026-10-23 (owner). The accept button is a plain tick, not a green one; the check's wording was wrong. Left to re-check after the area selection switched to exact sizes:

1. At 125% display scaling, draw any rectangle in the area dialog, type `100x100`, press Enter twice: the saved file is exactly 125x125 (before the change it could be 125x126). At 150% it is 150x150.
