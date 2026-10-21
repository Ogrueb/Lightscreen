# Area capture after S01 (dialog moved out of `tools/screenshot`)

The area dialog is now opened by the main window instead of by the screenshot itself. Use the `lightscreen-win64` artifact of the S01 PR's CI run, with the area action on a hotkey (or the tray menu's "Area").

1. **Without autoclose** (Options > "Area" autoclose off). Take an area screenshot, drag a rectangle, click the green tick: the saved file is the selected rectangle.
2. **With autoclose** on: the screenshot is saved as soon as the mouse is released.
3. **Magnifier.** In the area dialog, `Ctrl+M` turns the magnifier on and off (it starts in the state set in Options).
4. **Keyboard size.** Draw any rectangle, type `100x100` and press Enter: the selection becomes 100x100. Press Enter again to take it; the saved file is 100x100 (times the display scale).
5. **Cancel.** Open the area dialog and press Escape: nothing is saved, and Lightscreen is ready for the next screenshot (take a whole-screen one to check).
6. **Refresh.** With a window changing behind the dialog (a clock with seconds, a playing video), press F5: the dialog shows the current screen.
