# 2026-10-14: Stage 5, bug 26 fixed (hotkey from the keyboard)

- `HotkeyWidget`: with Tab or Shift+Tab focus, Space, Return and Enter start hotkey entry ("Type your hotkey", keyboard grabbed), the same as a click; other keys are still ignored, so tabbing through the options dialog cannot change a hotkey. Tab and Shift+Tab move on instead of being swallowed, and leaving the widget no longer moves focus to its parent (it used to, so Tab could never reach the next control). The Tab-focus flag is cleared on focus out.
- `tst_hotkeywidget`: `tabFocusIgnoresKeys` flipped (the flag no longer outlives focus); new `tabFocusThenKeyStartsEntry` (Space, Return, Enter) and `tabFocusMovesOnWithTab`. All failed before the fix. All suites pass under Xvfb on Linux.
- Needs a short check by hand on Windows (keyboard focus and `grabKeyboard` are platform code): see the PR.
