# 2026-10-14: Stage 5, bug 29 (`--quit` with no instance running)

- `lightscreen --quit` with no instance running now exits with status 0 instead of leaving a hidden instance. `executeArgument("--quit")` queues the quit (`QTimer::singleShot(0, qApp, &QCoreApplication::quit)`), so it survives `main` running the arguments before `exec()`. The other flags and `main.cpp` are unchanged.
- New test `tst_mainwindow::quitFlagBeforeEventLoopQuits` (fails before the fix: `exec()` ran until the 3 s guard). The CI Linux smoke gained a second step: `timeout 10 ./lightscreen --quit` with no instance running must exit 0. `docs/build.md` describes it.
- Not changed: `--quit` still bypasses `LightscreenWindow::quit` (no confirmation while processing or uploading, no saved position, no `finished`), as recorded by `quitFlagQuitsTheApplication`.
