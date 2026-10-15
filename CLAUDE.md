# Lightscreen fork

A fork of the unmaintained Lightscreen screenshot tool (C++ / Qt 5 / qmake), being modernized in stages. See docs/PLAN.md.

Before starting: read docs/PLAN.md, docs/decisions.md and docs/status.md (each explains its folder of entries).
Before finishing: add a new file docs/status/YYYY-MM-DD-<topic>.md (and docs/decisions/YYYY-MM-DD-<topic>.md if you made a decision; docs/known-bugs/NN-<topic>.md for a new bug). Never edit the index files docs/status.md, docs/decisions.md or docs/known-bugs.md to add an entry, and don't edit other threads' entries: one file per entry keeps parallel PRs free of merge conflicts.
A fix that can only be verified by hand on Windows adds its check as docs/windows-qa/YYYY-MM-DD-<topic>.md (see docs/windows-qa.md) instead of asking the owner for a per-PR check; the owner runs the whole list in one pass at the end of a stage.
Do not change behavior covered by the safety-net tests unless the task says so.

How to build: docs/build.md.
