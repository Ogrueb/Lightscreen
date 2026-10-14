# Decisions

Decisions, their reasons and the alternatives rejected. Newest first.

## 2026-10-14: Vendor SingleApplication and UGlobalHotkey instead of using submodules

**Decision.** The two git submodules were replaced by plain copies of their source in the repository, at the same paths (`tools/SingleApplication`, `tools/UGlobalHotkey`) and at the commits that were pinned (`ckaiser/SingleApplication@c6378ee`, `ckaiser/UGlobalHotkey@231b101`). The files are byte-identical to those commits. `.gitmodules` is gone. Each directory has a `VENDORED.md` recording its origin, commit and license (MIT for SingleApplication, public domain for UGlobalHotkey).

**Why.** Both submodules pointed at the original author's own forks, which are as unmaintained as the app. Every clone of the fork depended on those dormant repos staying online. The libraries are small (about 1,100 lines together) and already diverge from any maintained upstream, so there is nothing to gain from tracking them as separate repos. Keeping the same paths meant no build file or `#include` had to change.

**Rejected.**
- Forking both repos under the fork's account: keeps their history, but adds two repos to maintain for little code.
- Switching SingleApplication to the maintained `itay-grudev/SingleApplication` now: `main.cpp` uses this copy's custom API (`instanceArguments`), so it is a code change. Revisit with the Qt 6 decision.
