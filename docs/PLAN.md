# Modernization Plan

Fork and modernize an unmaintained (~5 years) FOSS project: fix the known bugs, then restructure it for long-term resilience.

## How this plan works

- **The repo is the record.** Every decision and handoff lives in `docs/` on `main`. Chats, cloud sessions and Project threads all start from the repo, so nothing has to be passed along by hand.
- **Merge handoff docs to `main` before the next stage starts.** Cloud sessions and Project threads each work on their own branch. A doc that sits on an unmerged branch is invisible to the next session.
- **Every session reads the docs first and updates them last.** `CLAUDE.md` tells Claude Code sessions to do this. A chat may not read `CLAUDE.md` on its own, so its prompt should say to.
- **Write conclusions, not transcripts.** Each handoff doc holds what the next stage needs: decisions, status, open questions.

## Repo docs

| File | Purpose | Written in |
|---|---|---|
| `CLAUDE.md` | Standing rules: conventions, constraints, "read `docs/` first, update on exit" | Stage 1, kept current |
| `docs/PLAN.md` | This document | Now |
| `docs/goals.md` | What "resilient" means here, compatibility commitments, out-of-scope items | Before stage 3 (by you) |
| `docs/known-bugs.md` | Technical description of each known bug (one file per bug in `docs/known-bugs/`) | Before stage 5 (by you) |
| `docs/build.md` | How to build and test; environment notes | Stage 1 |
| `docs/inventory.md` | Survey of the codebase as it stands | Stage 2 |
| `docs/modernization-plan.md` | Steps the size of one PR each | Stage 3 |
| `docs/decisions.md` | Decisions, reasons, rejected alternatives (one file per decision in `docs/decisions/`) | Stage 3 onward |
| `docs/windows-qa.md` | Manual Windows checks, run in one pass per stage (one file per check in `docs/windows-qa/`) | Stage 5 onward |
| `docs/status.md` | What's done, what changed from the plan, open items (one file per entry in `docs/status/`) | Stage 4 onward |

Suggested `CLAUDE.md` lines (the current wording in `CLAUDE.md` adds the one-file-per-entry rule):

```
Before starting: read docs/PLAN.md, docs/decisions.md and docs/status.md.
Before finishing: update docs/status.md (and docs/decisions.md if you made a decision).
Do not change behavior covered by the safety-net tests unless the task says so.
```

---

## Stage 1: Fork and get it building

**Where:** Local Claude Code or a chat · **Model:** Sonnet / Opus

Get the fork building and its existing tests running on a current toolchain, with as few changes as possible. Old dependencies usually need back-and-forth troubleshooting, so this stage is hands-on.

> **Handoff → cloud sessions**
> - Fork on GitHub; building on `main`
> - `docs/build.md`: toolchain versions, system dependencies, setup commands, what broke and how it was fixed, known flaky tests
> - Setup commands also entered in the cloud environment configuration, so cloud sessions can install dependencies
> - `CLAUDE.md` created
> - You: write `docs/goals.md` and `docs/known-bugs.md` (needed by stages 3 and 5)

## Stage 2: Inventory

**Where:** Plain cloud session (uses the credit) · **Model:** Sonnet

Survey the codebase mechanically. Record findings only, no judgments.

> **Handoff → chat (Fable)**
> - `docs/inventory.md` merged to `main`, covering:
>   - Dependencies: current version vs. latest, status (maintained / deprecated / abandoned)
>   - Module map and how modules depend on each other
>   - Uses of deprecated language or library APIs
>   - Test coverage per module
>   - State of build, CI and packaging
> - `docs/goals.md` and `docs/known-bugs.md` present (from you)
> - Chat connected to the repo with write access, so it can commit the plan

## Stage 3: Modernization plan

**Where:** Chat (Max Fable allowance; runs with your machine off) · **Model:** Fable

Give Fable the full repo plus `inventory.md`, `goals.md` and `known-bugs.md`. Ask for a staged plan that keeps the code working after every step. Start at the beginning of a weekly usage cycle.

Each step in the plan should be:
- the size of one PR
- listed with the steps it depends on and the files it touches
- paired with a done-check that uses the stage 4 tests
- tagged *mechanical* or *structural*

The plan should also include an overall merge order.

> **Handoff → cloud sessions**
> - `docs/modernization-plan.md` and `docs/decisions.md` merged to `main`
> - Test priorities: which modules need safety-net tests first, and which are marked for replacement (light testing only)
> - Bug triage: which known bugs to fix now (code being kept) and which to defer (code being replaced)

## Stage 4: Safety-net tests

**Where:** Plain cloud sessions in parallel (uses the credit) · **Model:** Sonnet

One session per module, in the priority order from stage 3. The tests record current behavior, not intended behavior. These tests let stage 6 restructure the code without silently changing what it does.

## Stage 5: Known bugs

**Where:** Plain cloud sessions in parallel (uses the credit) · **Model:** Opus

One session per bug marked "fix now". First write a failing test from the description in `known-bugs.md`, then fix the bug.

> **Escalation → chat (Fable), only for bugs Opus can't reproduce or fix**
> - The bug description
> - The failing test, if one exists
> - What Opus tried and why it failed (from that session's notes in `docs/status.md`)

> **Handoff → Project**
> - All test and bug-fix PRs merged to `main`
> - `docs/status.md`: test coverage achieved, bugs fixed or deferred, plan steps affected by the fixes
> - `docs/modernization-plan.md` updated if the fixes changed anything
> - Project setup:
>   - instructions point at `CLAUDE.md` and `docs/`
>   - coordinator model: Opus
>   - worker model: Sonnet for the first round
>   - same cloud environment as before

## Stage 6: Carry out the plan

**Where:** Project · **Model:** Coordinator Opus; workers Sonnet, then Opus

The coordinator splits `modernization-plan.md` into threads and manages merge order and overlaps. Workers share one model setting, so the work runs in rounds:
1. **Mechanical round:** Sonnet workers (dependency upgrades, CI, linting, packaging)
2. **Structural round:** Opus workers (module restructuring, interface changes)

If any single step needs Fable, take it out of the Project and run it in a chat, then merge before the next round.

When a decision changes, update `docs/decisions.md`, not only the coordinator's memory, so the Project and the repo stay in sync.

> **Handoff → local review**
> - The coordinator's recommended merge order
> - `docs/status.md` updated for each step, including any changes from the plan
> - `docs/decisions.md` complete
> - Draft `CHANGELOG`
> - List of deferred items and open issues

## Stage 7: Review and release

**Where:** Local Claude Code · **Model:** Opus / Sonnet

You review and merge. Use Claude for polishing only: README, docs, changelog, release notes. Tell it to polish without re-deciding anything the plan settled.

---

## Budget and timing

- **The cloud credit expires November 4.** Stages 2, 4 and 5 are its intended use, so run them first.
- **Fable (stage 3, plus any escalations):** uses the Max weekly allowance, capped at 50% of weekly limits. Start stage 3 at the beginning of a weekly cycle.
- **Projects use up limits quickly.** Start stage 6 the week after the Fable run.

## Still to verify

- [ ] You have the new Projects format (the beta is still rolling out)
- [ ] Project threads don't draw from the cloud credit (check Settings > Usage after one test thread)
- [ ] Whether changing the worker model affects running threads or only new ones
- [ ] Whether a single Project thread can be given its own model
