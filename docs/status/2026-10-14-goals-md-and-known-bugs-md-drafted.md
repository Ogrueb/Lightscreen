# 2026-10-14: goals.md and known-bugs.md drafted

- `docs/goals.md`: purpose, the six-point definition of "resilient", platform and compatibility commitments, out-of-scope list. Decisions taken by the owner in the drafting thread: Qt 6, Windows 10 minimum, Linux X11 kept as a CI-tested target, update check moved to GitHub Releases. Recorded in `docs/decisions.md`. Only the fork's name is still open, and it is not needed before Stage 3.
- `docs/known-bugs.md`: ten entries. Three confirmed from the code (window-picker icon handle, `extension()` missing return, missing Linux error sound), five inferred from the code and awaiting reproduction (mixed-DPI desktop capture, window capture margins, silent update failure, dead SSL workaround, dead macOS block), two reported by the owner (HDR browser brightness, PNG optimization queue).
- Upstream's issue tracker is disabled, so there was no issue list to mine. `docs/upstream-prs.md` (being written by another thread) may add entries from the five unmerged upstream PRs; fold those in when it lands.
- Stage 3 can start once this PR is merged.
