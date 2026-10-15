# 2026-10-14 (Stage 5, bug 26): keyboard entry in the hotkey widget

- **Space and Enter start hotkey entry directly; `clicked` is not emitted.** Nothing connects to it, and passing the key to `QPushButton` would emit `clicked` on the key release, after entry had started. *Rejected:* letting the keys through to `QPushButton` and starting entry from `clicked` (a mouse click would then start entry twice, once from `FocusIn` and once from `clicked`).
- **Shift+Tab focus counts as Tab focus, and focus out no longer calls `parentWidget()->setFocus()`.** Both are keyboard traps that would leave the bug half fixed: Shift+Tab onto the button grabbed the keyboard, and Tab off it bounced focus to the parent. Finishing an entry (a key recorded, or Escape) still hands focus to the parent, as before.
