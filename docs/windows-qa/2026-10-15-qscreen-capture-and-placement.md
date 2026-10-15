# Captures and window placement after QDesktopWidget removal (M05 PR, bug 11 re-test)

Needs two monitors for steps 2, 3 and 6. Compare with how 2.5 behaved; report any difference as a finding, not a blocker, unless something no longer works.

1. Whole screen capture: the file covers all monitors, as before.
2. Options, "current monitor" on, cursor on the non-primary monitor, whole screen capture: the file covers only that monitor (bug 11 re-test).
3. Area capture with "current monitor" off then on: the overlay covers all monitors, then only the monitor under the cursor; the help text sits on the primary monitor.
4. Active window capture, and the window picker: the picker opens centred on the monitor under the cursor.
5. Options dialog (first open, with no saved geometry) opens centred on its monitor; the main window reopens at its saved position.
6. With the preview enabled, the preview pops up in the chosen corner of the monitor it appears on.
