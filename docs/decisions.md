# Decisions

Decisions, their reasons and the alternatives rejected. Newest first.

## 2026-10-14: Qt 6, Windows 10 minimum, Linux kept, update check via GitHub Releases

**Decision.** The fork moves to Qt 6 (current LTS at the time of the port). Minimum Windows is 10; 32-bit builds, XP/Vista/7/8 code paths and the XP linker flags go. Linux X11 stays a target, tested by CI and cloud threads rather than by the owner. The update check is pointed at the fork's GitHub Releases instead of `lightscreen.com.ar`. Full statement in `docs/goals.md`.

**Why.** Qt 5.15 has no further open-source releases, so a fork on it would be "resilient" only until the next OS or compiler change broke Qt 5; the port cost (winextras/x11extras replacements plus about 20 small API changes) is one mechanical plan step. Qt 6 forces Windows 10 and C++17, both acceptable to the owner. Linux is the only target CI and cloud threads can run end to end, so keeping it gives the safety-net tests somewhere to run. The GitHub Releases check keeps the existing user experience with no server to run.

**Rejected.**
- Staying on Qt 5.15: zero porting cost now, dead platform later.
- Windows 7 minimum: not supported by Qt 6.
- Dropping Linux or keeping it build-only: loses the only end-to-end test platform the project has.
- Removing the update check: kept as a fallback if the GitHub check proves a nuisance ("for now" was the owner's wording).

## 2026-10-14: Vendor SingleApplication and UGlobalHotkey instead of using submodules

**Decision.** The two git submodules were replaced by plain copies of their source in the repository, at the same paths (`tools/SingleApplication`, `tools/UGlobalHotkey`) and at the commits that were pinned (`ckaiser/SingleApplication@c6378ee`, `ckaiser/UGlobalHotkey@231b101`). The files are byte-identical to those commits. `.gitmodules` is gone. Each directory has a `VENDORED.md` recording its origin, commit and license (MIT for SingleApplication, public domain for UGlobalHotkey).

**Why.** Both submodules pointed at the original author's own forks, which are as unmaintained as the app. Every clone of the fork depended on those dormant repos staying online. The libraries are small (about 1,100 lines together) and already diverge from any maintained upstream, so there is nothing to gain from tracking them as separate repos. Keeping the same paths meant no build file or `#include` had to change.

**Rejected.**
- Forking both repos under the fork's account: keeps their history, but adds two repos to maintain for little code.
- Switching SingleApplication to the maintained `itay-grudev/SingleApplication` now: `main.cpp` uses this copy's custom API (`instanceArguments`), so it is a code change. Revisit with the Qt 6 decision.
