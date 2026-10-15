# 24. Typed area size ignores an upper-case X

- **Found:** found while writing the Stage 4 area dialog tests.
- **Where:** `dialogs/areadialog.cpp:227-240` (`keyPressEvent`) and `:137-163` (`keyboardResize`).
- **What happens:** `keyPressEvent` lets the X key through whatever its text, so Shift+X or Caps Lock adds `X` to the typed size, but `keyboardResize` only splits on a lower-case `x`. Typing `100X100` and Enter shows the text on screen, then silently does nothing. The "only one x" check is case-sensitive too, so `1X2x3` can be typed.
- **Fix:** append a lower-case `x` for the X key (or split case-insensitively).
- **Platform:** any. **Test:** `tst_AreaDialog::typing_upperCaseX` and the "upper case X" row of `keyboardResize` record it. Confirmed by the test.
- **Triage:** fix now (session F).
- **Status:** fixed in Stage 5. `keyPressEvent` stores the X key as a lower-case `x` whatever its text, and refuses a second one of either case. `tst_AreaDialog::typing_upperCaseX` now asserts `100X50` resizes to 100x50; the `keyboardResize` "upper case X" row is kept and is no longer reachable by typing.
