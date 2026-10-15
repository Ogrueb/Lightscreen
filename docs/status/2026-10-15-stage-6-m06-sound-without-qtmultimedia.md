# 2026-10-15: Stage 6, M06 done (sound without QtMultimedia)

- `QSound` is gone and `multimedia` is out of `QT`. New `os::playSound(os::Sound)`: on Windows, `PlaySoundW` from `winmm` (added to `LIBS`), async; the success sound plays from the bytes of `:/sounds/ls.screenshot.wav` (new qrc entry), the error sound is the system default sound as before (`docs/decisions/2026-10-15-m06-error-sound-is-the-system-default.md`). Linux: a no-op, as before (the options dialog hides the checkbox there).
- Bug 3: closed on Windows, the only platform where the option exists (the success sound no longer depends on the working directory). Linux has no sound option, so the missing `sound/ls.error.wav` no longer matters.
- CI: the Linux job no longer installs `qtmultimedia5-dev`; `docs/build.md` updated to match.
- Tests: `tst_os` checks the resource is a non-empty WAV and that `playSound` returns. Linux: built without QtMultimedia installed, smoke passes, all 11 test suites pass.
- Windows QA: `docs/windows-qa/2026-10-15-sound-cues.md`.
- Did not depend on M05 (PR #38); branched from master. The two overlap only in the include lists of `tools/os.cpp` and `lightscreenwindow.cpp`.
