# WEBP screenshots are saved (PR #29, bug 19)

Optional and short.

1. In Options, choose WEBP as the format, save, and take a screenshot: a `.webp` file is written and opens in a viewer. (It needs Qt's `qwebp` plugin, which `windeployqt` deploys when the Qt install has it; if no file appears, note that.)
2. Reopen Options: the format still shows WEBP.
