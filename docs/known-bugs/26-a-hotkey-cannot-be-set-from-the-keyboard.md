# 26. A hotkey cannot be set from the keyboard

- **Found:** found while writing the Stage 4 hotkey widget tests.
- **Where:** `widgets/hotkeywidget.cpp:85-91` (`event`, `FocusIn`) and `:110-112` (`keyPressEvent`).
- **What happens:** when the hotkey button gets focus with Tab, it sets `mKeyboardFocus` and `keyPressEvent` then returns early for every key. Space and Enter never reach `QPushButton`, so the button cannot be pressed, and no other key is recorded: only a mouse click (any non-Tab focus) starts hotkey entry. Keyboard-only users cannot change a hotkey. The flag also stays set after focus leaves, until the next non-Tab focus.
- **Fix:** let Space and Enter through when `mKeyboardFocus` is set, and have them start hotkey entry (the same as a click: "Type your hotkey" and `grabKeyboard`).
- **Platform:** any. **Test:** `tst_HotkeyWidget::tabFocusIgnoresKeys` asserts that Space and Enter do nothing; the fix flips that assertion. Confirmed by the test; not tried in the running app.
- **Triage:** fix now (session H).
- **Status:** fixed in Stage 5 (2026-10-14). Space, Return and Enter on a button reached with Tab (or Shift+Tab) start hotkey entry as a click does; other keys are still ignored there. Two more keyboard traps found while testing were fixed with it: Tab never got past the button (leaving it handed focus to the parent widget), and Shift+Tab onto it started hotkey entry. The flag is cleared when focus leaves. Tests: `tabFocusIgnoresKeys`, `tabFocusThenKeyStartsEntry`, `tabFocusMovesOnWithTab`.
