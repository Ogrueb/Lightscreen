# 2026-10-14: M01 GitHub Actions CI

- `.github/workflows/ci.yml`: two jobs on every pull request and push to `master`. `linux` (ubuntu-latest, Qt 5.15 from apt) builds and runs the Linux smoke (start under Xvfb in portable mode, stop with `--quit`, expect exit 0 within ten seconds). `windows` (windows-latest, Qt 5.15.2 msvc2019_64, MSVC x64) builds, runs `windeployqt` and uploads `lightscreen-win64` as an artifact.
- README: the upstream AppVeyor and Coverity badges are replaced by the fork's CI badge (the rest of the README is M14's).
- `docs/build.md`: Linux section now verified, plus a "Continuous integration" section. Details the plan left open are in `docs/decisions.md` (M01 entry).
- Verified in the cloud container: the Linux build and the smoke script pass locally (Ubuntu 24.04, Qt 5.15.13). Both jobs green on PR #7; the Windows job produced the `lightscreen-win64` artifact (about 42 MB).
- Still open from the done-check: the optional one-time Windows QA (download the artifact and start it on the owner's machine).
- M02 can build on this: its `make check` steps go into the same two jobs.
