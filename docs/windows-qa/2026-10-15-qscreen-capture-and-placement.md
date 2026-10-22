# Captures and window placement after QDesktopWidget removal (M05 PR)

Needs two monitors for steps 2 and 5. Compare with how 2.5 behaved; report any difference as a finding, not a blocker, unless something no longer works. Capture sizes and "Grab only the active monitor" are checked in `2026-10-22-mixed-dpi-capture.md`.

1. Whole screen capture: the file covers all monitors, as before.
2. Area capture with "Grab only the active monitor." off then on (Options tab, Screenshots): the overlay covers all monitors, then only the monitor under the cursor; the help text sits on the primary monitor.
3. Active window capture, and the window picker: the picker opens centred on the monitor under the cursor.
4. Options dialog (first open, with no saved geometry) opens centred on its monitor; the main window reopens at its saved position.
5. With the preview enabled, the preview pops up in the chosen corner of the monitor it appears on.
