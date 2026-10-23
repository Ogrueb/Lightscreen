# Vendored copy

Copied unchanged from https://github.com/ckaiser/UGlobalHotkey at commit
`231b10144741b29037f0128bb7a1cd7176529f74` (2016-08-07), which this repository
previously pinned as a git submodule. That repository is the original
Lightscreen author's fork of https://github.com/falceeffect/UGlobalHotkey.

License: Public Domain, as stated in the "License" section of `README.md` in
this directory. The upstream repository has no separate license file.

Any change to these files after the copy is recorded in the parent repository's
history and in `docs/decisions.md`.

## Local changes

- **M04 (2026-10-15).** `unregisterAllHotkeys` iterates over a copy of the
  registered ids with a range-based `for` instead of `foreach` (which Qt 6
  deprecates); `unregisterHotkey` removes from the container being walked, so
  the copy is what `foreach` made implicitly.
- **M08 (2026-10-16), Linux only.** The constructor gets the xcb connection from
  `QX11Info::connection()` on Qt 5 and from
  `QNativeInterface::QX11Application::connection()` on Qt 6, instead of
  `QPlatformNativeInterface` (a private Qt header). `uglobalhotkey-sources.pri`
  and `uglobalhotkey.pro` therefore add `x11extras` on Qt 5 instead of
  `gui-private`. `nativeEventFilter`'s result parameter is the new
  `UNativeEventResult` typedef in `uglobalhotkeys.h`: `long` on Qt 5, `qintptr`
  on Qt 6. The Windows `nativeEvent` signature is unchanged (M09).
- **M09 (2026-10-20), Qt 6.** The `UNativeEventResult` typedef now applies on
  every platform, and the Windows `nativeEvent` and `winEvent` take it (`qintptr`
  on Qt 6), with `override` on `nativeEvent`: with the old `long *` signature
  Qt 6 would never call it and global hotkeys would stop working on Windows.
  `UKeySequence::fromString` takes the key with `QKeyCombination::key()`, since
  `QKeySequence::operator[]` returns a `QKeyCombination` on Qt 6.
- **M10 (2026-10-21), Qt 5 branches removed.** Qt 6.2 is the minimum, so the
  `QT_VERSION` checks are gone: the constructor always takes the xcb connection
  from `QNativeInterface::QX11Application`, `UNativeEventResult` is always
  `qintptr`, and `uglobalhotkey-sources.pri` and `uglobalhotkey.pro` no longer
  add `x11extras` (the standalone `.pro`, which the app does not use, also asks
  for C++17 and `widgets` unconditionally).
- **M11 (2026-10-21), CMake.** The qmake files (`uglobalhotkey.pri`, the three
  `uglobalhotkey-*.pri` and the standalone `uglobalhotkey.pro`) are replaced by
  `CMakeLists.txt`, a static library with the same sources, the same
  `UGLOBALHOTKEY_NOEXPORT` define and the same libraries (`user32` on Windows,
  `xcb` and `xcb-keysyms` on Linux). No source file changed.
- **Bug 33 (2026-10-23), Windows only.** Hotkeys are registered to the thread
  (`RegisterHotKey(nullptr, ...)`) instead of to the widget's own handle
  (`winId()`), and `WM_HOTKEY` is caught by a `QAbstractNativeEventFilter`
  (installed on `qApp`, as on Linux) instead of `QWidget::nativeEvent`, which is
  removed on Windows along with `winEvent`. Registered to the widget, hotkeys
  stopped arriving on Qt 6 while the main window was hidden to the tray. The
  filter ignores `WM_HOTKEY` ids this object did not register (upstream asserted
  on them). Test: `tests/tst_globalhotkeys`.
