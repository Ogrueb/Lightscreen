# 2026-10-21: Stage 6, M15 release workflow (PR open, owner merges)

- New `.github/workflows/release.yml`. A `v*` tag builds the Windows package (Qt 6.8.3, MSVC, `windows-msvc-release` preset, tests, `windeployqt --release --no-translations`), zips it as `lightscreen-<version>-win64.zip` and creates a draft GitHub Release with the zip attached. `workflow_dispatch` is the dry run: a draft pre-release under `dry-run-<run number>`. A pull request that changes the workflow runs the build and the zip only. Details and reasons: `docs/decisions/2026-10-21-m15-release-workflow-details.md`.
- `docs/build.md` has a new "Releases" section.
- Verified: the PR's own run of the workflow (build, tests, zip) is green. The draft-release job can only run once the workflow is on `master` (GitHub offers "Run workflow" only for workflows on the default branch), so the plan's done-check, a `workflow_dispatch` dry run, happens after the merge; its Windows QA is `docs/windows-qa/2026-10-21-release-dry-run.md`.
- No tag was pushed. None should be before M16.
- Next in the plan: S01 (dialogs out of `tools/screenshot`), the start of the structural round; it can run in parallel with this PR.
