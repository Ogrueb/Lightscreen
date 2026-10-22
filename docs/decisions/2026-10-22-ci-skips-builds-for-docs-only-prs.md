# 2026-10-22: CI skips the builds for docs-only pull requests

- **Decision (owner, 2026-10-22):** a pull request whose changed files are all under `docs/` or end in `.md` runs only the docs job; the Linux and Windows builds are skipped. Pushes to `master` always build.
- **Why:** most Project threads end with a docs-only PR (status, decisions, QA files), and each one waited several minutes for two full Qt builds that cannot be affected by Markdown. The docs job is the check that matters for those PRs.
- **How:** the decision is made inside the existing docs job (a `git diff --name-only` against the merge commit's first parent), and the build jobs use `needs: docs` with an `if:` on its output. A skipped job counts as passing, so a branch-protection rule on the build checks would still be satisfied; a workflow-level `paths-ignore` would instead leave such required checks waiting forever.
- **Kept building on `master`:** the `lightscreen-win64` artifact from the latest green `master` run is what the owner downloads for the Windows QA pass, so it must exist for every commit, docs-only or not.
- **Rejected:** a third-party path-filter action (one more pinned dependency for a one-line `grep`); workflow-level `paths-ignore` (above).
- **Cost:** the build jobs now start after the docs job, about ten seconds later than before.
