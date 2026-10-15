# 3. Linux error sound file does not exist

- **Found:** confirmed from the code.
- **Where:** `lightscreenwindow.cpp:215`, `QSound::play("sound/ls.error.wav")`.
- **What happens:** on a failed capture with "play sound" enabled, non-Windows builds try to play `sound/ls.error.wav`. The repo has `sounds/` (plural) and it contains only `ls.screenshot.wav`, so nothing plays. Both sound paths are also relative to the current working directory, not the executable or a resource, so even the success sound is silent unless the app is started from its own directory.
- **Fix:** ship both sounds in `lightscreen.qrc` and play from `qrc:/`; pick or draw an error sound.
- **Platform:** Linux for the missing file; both for the relative path. **Test:** headless check that the resource exists.
