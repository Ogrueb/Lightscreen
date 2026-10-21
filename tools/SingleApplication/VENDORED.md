# Vendored copy

Copied unchanged from https://github.com/ckaiser/SingleApplication at commit
`c6378eec45a5fdf699b4d27fb4be22a190b2a184` (2016-06-07), which this repository
previously pinned as a git submodule. That repository is the original
Lightscreen author's fork of https://github.com/itay-grudev/SingleApplication.

License: MIT, see `LICENSE` in this directory.

Any change to these files after the copy is recorded in the parent repository's
history and in `docs/decisions.md`.

## Local changes

- **M04 (2026-10-15), APIs Qt 6 removed.** In `singleapplication.cpp`: the
  server name is cleaned with `QRegularExpression` (with
  `UseUnicodePropertiesOption`, so `\w` matches what `QRegExp` matched) instead
  of `QRegExp`; the `newConnection` connect uses member-function pointers
  instead of `SIGNAL()`/`SLOT()` strings; `Q_NULLPTR` became `nullptr`.
- No other change since vendoring; M09 and M10 needed none here.
