# Known bugs

Technical description of each known bug, one file per bug in `docs/known-bugs/`, named `NN-<topic>.md` (two-digit number, then a slug), so parallel PRs never edit the same file (see `docs/decisions/2026-10-15-shared-docs-one-file-per-entry.md`). "Bug N" in code, tests and other docs is the file whose name starts with that number. Each entry says where the bug is, how it shows, how to reproduce it, which platform can reproduce it (cloud threads can only run Linux), how it was triaged and, once fixed, its status.

- **Read:** `ls docs/known-bugs` for the list, `cat docs/known-bugs/NN-*.md` for one bug.
- **Add:** take the next number above every bug on `master` and in open PRs (`gh pr list`, then the PRs' file lists), create `docs/known-bugs/NN-<topic>.md` starting with `# N. <title>` and a `- **Found:**` line, and do not list it here. CI (`.github/scripts/check-docs.sh`) fails on a duplicate number; if it does, the later PR renumbers its bug.
- **Update:** a fix edits only its own bug's file (the **Status:** line).

The triage (fix now or defer, and where) is in `docs/modernization-plan.md`, section 7. Line references in bugs 1 to 14 are to `master` after PR #3.

## Notes carried over from the single-file list

- Sources: bugs 1-3 were confirmed from the code; 4-8 inferred from the code and still need a reproduction; 9-10 reported by the owner on 2026-10-14; 11-12 come from the upstream pull requests (`docs/upstream-prs.md`), since upstream's issue tracker is disabled; 13-14 found while writing the Stage 3 plan; 15-29 by the Stage 4 safety-net tests.
- Upstream's unmerged Linux build fixes (#40, #44) need no entry: `#undef Success` is already in `tools/screenshot.cpp:47`, and the fork builds on Linux. #37 (Print Screen hotkey registered as id 0) was merged upstream and is in the fork.
- Bugs 15 to 18 are recorded as current behaviour in `tests/tst_screenshot`; the test names in their entries are the assertions a fix flips.
- Still to add: bugs the owner has hit on their own machine.
