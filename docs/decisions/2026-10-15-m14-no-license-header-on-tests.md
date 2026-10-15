# 2026-10-15: M14, no license header on the test sources

**Decision.** M14 added the GPL v2 header to the 12 app files the plan listed and not to the test sources under `tests/` (`tst_*.cpp`, `optipng_stub/main.cpp`). The M14 done-check ("every first-party `.cpp`/`.h` begins with the GPL v2 header") is read as covering the application's sources, which is what `docs/goals.md` ("Files with no license header get one") and the inventory meant.

**Why.** The existing header names Christian Kaiser as copyright holder, which is wrong for code written in this fork, and choosing a copyright line for the fork's own code belongs with the rename (M16), when the about text and `version.pri` copyright strings change too. The tests are not shipped. The repository's `LICENSE` (GPL v2) covers them either way.

**Rejected.** Copying the Christian Kaiser header onto the tests (misattributes); inventing a "fork contributors" line now (pre-empts the M16 naming decision).
