# 2026-10-15 (repo mechanics): shared docs are one file per entry

**Decision.** `docs/status.md`, `docs/decisions.md` and `docs/known-bugs.md` become short index files that explain the layout; the entries live one per file in `docs/status/`, `docs/decisions/` (`YYYY-MM-DD-<topic>.md`) and `docs/known-bugs/` (`NN-<topic>.md`). A PR adds its own files and edits only its own bug's file. `.github/scripts/check-docs.sh`, run as the `Docs layout` CI job, checks the names and fails on a duplicate bug number.

**Why.** Every PR appended to `status.md`, many to `known-bugs.md` and `decisions.md`, all at the same spot, so each merge left every other open PR in conflict and the owner waited on a rebase and CI per merge. Two PRs that add different files cannot conflict in git, whichever merge engine GitHub uses. Fixes edit their bug's **Status:** line in place, which a separate file per bug also keeps apart.

**Rejected.**
- `.gitattributes` `merge=union` on the three files: union only helps pure appends. It garbles in-place edits (a fix's **Status:** line, a decision amended) by keeping both sides' lines, it gives no conflict when two PRs really do clash, and whether GitHub's merge button and mergeability check apply a merge driver from `.gitattributes` is not documented. Not tested on GitHub, because the per-file layout is conflict-free without depending on it.
- An index that lists every entry: each PR would append a line to it and conflict again. The folder listing is the index.
- Bug numbers assigned at merge time: needs tooling or a human step per merge. Two PRs taking the same number is rare and the CI check catches it.

**Addition, same day (owner's request): Windows QA in one pass.** Threads used to ask the owner to download the Windows build and check each PR by hand. Those checks now go into `docs/windows-qa/`, one file per check, and the owner runs them all at the end of a stage. One file per check rather than one shared list, for the same reason as above. *Rejected:* a single `docs/windows-qa.md` with a section per PR (every PR would edit it at the same spot).
