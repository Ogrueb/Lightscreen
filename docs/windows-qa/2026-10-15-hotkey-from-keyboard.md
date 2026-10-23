# A hotkey can be set from the keyboard (PR #31, bug 26)

Steps 1 and 2 (Space starts entry, Alt+F5 is recorded) passed on 2026-10-23 (owner). Step 3 found bug 34. Left to re-check, in Options > Hotkeys:

1. Tab to a hotkey button (it is only highlighted), press Tab again: focus moves to the next control and the hotkey is unchanged. Shift+Tab comes back, again without changing it.
2. Tab to a hotkey button, press Space ("Type your hotkey"), then press Tab: the button shows its old hotkey again and focus moves on. The same with Shift+Tab.
3. Press Space on a hotkey button, then Ctrl+Tab: the hotkey becomes Ctrl+Tab (combinations are still allowed). Set it back afterwards.
