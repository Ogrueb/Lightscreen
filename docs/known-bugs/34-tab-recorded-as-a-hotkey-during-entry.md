# 34. Tab is recorded as a hotkey while a hotkey button waits for one

- **Found:** by the owner in the Windows QA pass (A5, `2026-10-15-hotkey-from-keyboard.md` step 3), 2026-10-23.
- **Where:** `HotkeyWidget::keyPressEvent` (`widgets/hotkeywidget.cpp`).
- **What happens:** a hotkey button has two states:
  - Reached with Tab, it is only highlighted, and Tab and Shift+Tab move focus on (bug 26's fix).
  - Once it shows "Type your hotkey" (after Space, Enter or a click), it records the next key, Tab included. A keyboard user tabbing on from there set the hotkey to Tab, a key that would then be grabbed in every program.
- **Fix:** in entry mode, a plain Tab or Shift+Tab (`Key_Tab`, or `Key_Backtab`/`Key_Tab` with Shift) ends the entry with the hotkey unchanged, releases the keyboard and moves focus on (`focusNextPrevChild`). Tab with other modifiers (Ctrl+Tab, Alt+Tab) is still recorded.
- **Test:** `tst_HotkeyWidget::tabDuringEntryMovesOn` (Tab, Backtab, Shift+Tab) fails before the fix and passes after it; `ctrlTabDuringEntryIsRecorded` keeps the combination case.
- **Cause inferred from the owner's step:** the check's wording let step 3 start with the button still in entry mode. If the owner saw Tab captured with the button only highlighted, that is a different path, which the re-check will show.
- **Platform:** any. **Status:** fixed (2026-10-23), awaiting the owner's re-check.
