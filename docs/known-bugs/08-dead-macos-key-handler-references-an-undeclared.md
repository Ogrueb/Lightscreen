# 8. Dead macOS key handler references an undeclared variable

- **Found:** inferred from the code, reproduction needed.
- **Where:** `lightscreenwindow.cpp:1000-1010`, under `#ifdef Q_WS_MAC`.
- **What happens:** `Q_WS_MAC` is a Qt 4 macro, so the block never compiles; if it did, `e->key()` refers to a variable that does not exist. Not a runtime bug; remove with the Qt 6 port.
- **Platform:** n/a.
