# 2026-10-15: M06, the error sound stays the system default sound

- **Decision:** on Windows a failed capture plays `SND_ALIAS_SYSTEMDEFAULT` (the "Default Beep" in Sound settings), not `SystemAsterisk` as the plan's M06 text suggests.
- **Why:** the old code played a path that does not exist, and `PlaySound`, which `QSound` used underneath, falls back to the system default sound in that case. Keeping that alias makes M06 a pure swap with no audible change. `SystemAsterisk` is a different sound ("Asterisk") and would be a behaviour change nobody asked for.
- **Also:** the success sound is now read from `:/sounds/ls.screenshot.wav` instead of a path relative to the working directory, so it plays whatever folder the app starts from (the Windows deploy never carried `sounds/`, so it was silent before unless started from the source tree). That is bug 3's fix on the only platform with the option; Linux still hides the checkbox and plays nothing.
