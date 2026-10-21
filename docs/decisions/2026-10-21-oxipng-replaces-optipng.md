# 2026-10-21 (evaluation, before S05): oxipng replaces optipng as the PNG optimizer, done inside S05

**Decision (recommended, owner confirms when S05 starts).** Switch the "optimize PNG" option from `optipng` to `oxipng`. Do the code change inside S05 (the optimization queue), not as its own step, and have the Windows zip ship a pinned `oxipng.exe` from M15 on (and from the CI deploy step, so the `lightscreen-win64` artifact matches). On Linux, look for `oxipng` on `PATH` first and fall back to `optipng`, so existing Linux setups keep working. The settings key `options/optimize` stays as it is.

## What the app does today

- `Screenshot::optimize` (`tools/screenshot.cpp`) runs `optipng -out <file>.optipng.tmp <file>` and replaces the original only on exit code 0 (bug 10's Stage 5 fix). Linux finds `optipng` on `PATH`; Windows wants `optipng.exe` next to `lightscreen.exe`. `OptionsDialog` greys the checkbox out when the binary is missing.
- `optipng` is **not vendored and not bundled**: nothing under `tools/`, nothing in the CI deploy step (`windeployqt` only). So on a fork build for Windows the option is disabled out of the box today. Shipping any optimizer at all is new work, whichever one it is.
- Tests: `tests/optipng_stub` (a C++ stand-in that parses `-out`) and `tests/tst_optimize`; `tst_settings` checks the dialog's "not found" branch with `findExecutable("optipng")`.

## Findings (measured in the cloud container, Ubuntu 24.04, 4 cores; optipng 0.7.8, oxipng 10.2.1)

- **CLI.** `optipng -out <tmp> <file>` becomes `oxipng -q --out <tmp> <file>` (double dash; oxipng reads `-out` as `-o ut` and exits 2). Exit codes match what the app checks: 0 on success, non-zero on a bad input with no output written. Two differences: oxipng overwrites an existing `--out` file where optipng refuses (harmless, the app deletes the temporary file first), and on an already optimal file both still write the output and exit 0. Default level is `-o 2` in both.
- **Result.** On a 1920×1080 UI-like test image (windows, text, a noisy strip): optipng `-o2` 243.7 kB in 1.9 s; oxipng `-o2` 221.3 kB (9 % smaller) in 1.1 s; oxipng `-o4` 205.1 kB in 2.2 s; optipng `-o5` 221.9 kB in 9.5 s. On a flat synthetic image optipng `-o2` came out slightly smaller (6.1 kB vs 7.3 kB). One machine, two images: indicative, not a benchmark.
- **Threads.** oxipng uses every logical CPU by default (`-t/--threads`). With S05's one-process-at-a-time queue that is a short spike per file; S05 may pass `--threads` (for example half the cores) if the owner wants captures to stay smooth.
- **Size.** The Linux binary built with `cargo install oxipng --locked` (default features) is 1.4 MB, linking only libc, libm and libgcc_s. The Windows binary was not measured here; expect the same order, so the zip grows by roughly 1 to 2 MB (the Qt deploy is tens of MB).
- **Licence.** oxipng is MIT. Its dependencies are MIT/Apache-2.0 dual-licensed, except `libdeflater`, `libdeflate-sys` and `zopfli` (Apache-2.0 only). It runs as a separate process, so nothing is linked into the GPL v2 app; shipping it in the zip is aggregation. The zip needs the oxipng licence text and a third-party notice (a `LICENSES/` folder or a `THIRD-PARTY.md`), which M15 should add anyway for Qt. optipng (zlib licence) needed the same.
- **Linux availability.** Ubuntu 24.04's apt has no `oxipng` package (checked with `apt-cache policy`); other distributions carry it, and `cargo install oxipng` works. That is why Linux keeps the `optipng` fallback, and the dialog's hint names both.

## Cost, by area

- **Code (S05).** The binary lookup becomes a short candidate list (`oxipng`, then `optipng` on Linux; `oxipng.exe` on Windows), each with its own argument list; the temporary-file name loses "optipng"; `OptionsDialog` checks the same lookup and its label and tooltip (`optionsdialog.ui`) change. Rename the stub to a neutral optimizer stub that accepts both `-out` and `--out`, and keep one test per binary name. This changes behaviour the safety-net tests cover, so the owner merges, which S05 already requires.
- **Build.** None for CMake: the app never builds the optimizer, so M11 (merged) is unaffected. Building oxipng from source in CI (`cargo build --release --locked`, about 40 s here) is possible but adds a Rust toolchain to the Windows job for no gain.
- **CI and release.** Windows deploy (CI) and M15: download the pinned oxipng release zip for `x86_64-pc-windows-msvc`, check it against a SHA-256 recorded in the workflow, copy `oxipng.exe` and its licence into `deploy/`. A few lines per workflow; bumping oxipng means changing a version and a hash. Linux CI can `cargo install oxipng --locked` (cached) for an integration test, as S05 planned with optipng.
- **Update and release flow.** Unchanged: the update check reads the fork's own GitHub Releases, and `oxipng.exe` travels inside the zip. Releases stay unsigned; an unsigned helper exe next to an unsigned app changes nothing for SmartScreen. oxipng's Windows build runs on Windows 10.
- **Windows QA.** One entry when S05 lands: the option is enabled in the zip, a capture gets smaller, the original survives killing `oxipng.exe` mid-run.

## Why

oxipng is maintained (10.2.1, September 2026; optipng's last release, 0.7.8, is from 2023), gives smaller files at the same level in less time, and the app's interface to it is one process call that bug 10's fix already made robust. Since nothing is bundled today, the Windows packaging work is the same for either tool, so the swap itself costs only the argument list, the names and the stub.

## Rejected

- **Keep optipng only.** Still requires the same bundling work to make the option usable on Windows, for an older, slower tool.
- **Link oxipng as a library** (a Rust crate, so only through FFI). Adds a Rust toolchain to the CMake build and mixes Apache-2.0-only code into the GPL v2 binary; the process boundary avoids both.
- **Build oxipng from source in CI.** Reproducible, but a second toolchain on the Windows runner; a pinned, hash-checked upstream binary is enough for an unsigned release.
- **A separate step before S05.** It would rewrite `Screenshot::optimize` and the stub only for S05 to move them into `Optimizer` straight after.
