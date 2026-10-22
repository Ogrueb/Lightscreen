# 2026-10-22: Stage 7, CHANGELOG and v3.0.0 release notes drafted

- `CHANGELOG.md` (new, repository root, Keep a Changelog format): the 3.0.0 entry, compared with upstream 2.5, grouped as Changed, Added, Fixed, Removed, Known issues and For developers. Bug numbers point to `docs/known-bugs/`. Marked "unreleased" until the tag.
- `docs/release-notes/v3.0.0.md` (new): the text for the GitHub Release body, which the release workflow leaves as a draft for the owner to fill and publish. Highlights, install and upgrade steps, requirements, known issues.
- Bug tally used in both: 25 of 29 fixed in code (bugs 4, 5, 11 and 12 still await the Windows QA pass); 9 and 20 documented as known issues; 16's upload-only leftover not listed (no user-visible effect beyond notification timing); 3 closed by M06.
- Owner's calls this round: the Windows QA pass is on hold; the screenshot file prefix stays `screenshot.`; the fractional-scaling rounding in `main.cpp` is listed as a known issue, not changed.
- Before tagging `v3.0.0`: the Windows QA pass, then change "unreleased" to the release date in `CHANGELOG.md`.
