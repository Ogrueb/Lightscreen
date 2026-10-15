# 2026-10-15: Stage 5, bug 24 fixed (upper-case X in a typed area size)

- `AreaDialog::keyPressEvent` appends a lower-case `x` for the X key (Shift+X, Caps Lock), so `800X600` + Enter resizes the selection, and a second x of either case is refused. `keyboardResize` is unchanged.
- `tst_AreaDialog::typing_upperCaseX` flipped to the fixed behaviour (failed before the fix). All suites pass under Xvfb on Linux.
