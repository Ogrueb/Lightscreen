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

- **M08 (2026-10-16), Linux only.** The constructor gets the xcb connection from
  `QX11Info::connection()` on Qt 5 and from
  `QNativeInterface::QX11Application::connection()` on Qt 6, instead of
  `QPlatformNativeInterface` (a private Qt header). `uglobalhotkey-sources.pri`
  and `uglobalhotkey.pro` therefore add `x11extras` on Qt 5 instead of
  `gui-private`. `nativeEventFilter`'s result parameter is the new
  `UNativeEventResult` typedef in `uglobalhotkeys.h`: `long` on Qt 5, `qintptr`
  on Qt 6. The Windows `nativeEvent` signature is unchanged (M09).
