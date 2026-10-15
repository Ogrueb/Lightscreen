# 2026-10-14: Stage 3 plan written

- `docs/modernization-plan.md`: 16 mechanical steps (M01-M16) and 7 structural steps (S01-S07, two optional), each one PR with dependencies, files, a done-check and a merge owner; an overall merge order; Stage 4 test priorities with the seams they need; Stage 5 bug triage. Decisions and rejected alternatives are in `docs/decisions.md` (Stage 3 entry).
- Two plan steps come **before Stage 4**: M01 (GitHub Actions CI on ubuntu and windows-latest) and M02 (test harness, three small seams). Stage 4 cannot start until both are merged.
- Stage 5 sessions: bugs 1, 2, 5 (bounds only), 10 (local defects only) and the new bug 13. Everything else is deferred to a named plan step.
- Two new bugs found while reading the code are in `docs/known-bugs.md`: 13 (`updateHistory` inserts instead of updating; inferred) and 14 (history-dialog upload ignores the chosen service; confirmed).
- Owner decisions taken in the Stage 3 thread (plan section 8): the Windows jump list is dropped in the port, with S07 as an optional later re-addition; the `K/Lightscreen` settings identity is kept at the rename. The fork's name is still needed before M16.
- PR #4 (`docs/upstream-prs.md`, bugs 11 and 12) was already merged when this stage started; the plan is written against `master` at `d92b3ca`.
- Nothing was built in this thread: the cloud container has no Qt installed. The plan's API claims come from the code and the Qt documentation. M01 is where a build first runs in CI.
