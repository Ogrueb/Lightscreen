# Windows QA

Manual checks that only a person on Windows can do, collected so the owner runs them in one pass at the end of a stage instead of one download per PR. One file per check in `docs/windows-qa/`, named `YYYY-MM-DD-<topic>.md`, so parallel PRs never edit the same file (see `docs/decisions/2026-10-15-shared-docs-one-file-per-entry.md`).

- **Run the pass:** download the `lightscreen-win64` artifact from the latest green CI run on `master`, unzip it, then work through `cat docs/windows-qa/*.md` (or ask any thread to print the list). Tell a thread which checks passed and which failed.
- **After the pass:** a thread deletes the files of the checks that passed and updates the matching bug's **Status:** line; a failed check becomes a new bug in `docs/known-bugs/`.
- **Add (threads):** when a fix can only be verified by hand on Windows, add `docs/windows-qa/<today>-<topic>.md` in the same PR, starting with `# <title> (PR #N, bug N)` and listing short numbered steps with the expected result. Do not ask the owner for a per-PR check, and do not list the file here.
