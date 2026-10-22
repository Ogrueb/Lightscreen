# 2026-10-22 (Stage 6, S06): one default per setting, the options dialog's values, PNG as the default format

**Decision (owner, 2026-10-22).** Run S06. Every option the app reads gets one default, kept in `tools/settings.{h,cpp}` (`Settings::defaultValue`, `Settings::value`). Where the options dialog and the other readers disagreed, the dialog's value is the default, except `file/format`, which is PNG (0), not the dialog's JPEG (1). The owner accepted PNG.

**Why the dialog's values.** On first run the window opens the options dialog and saves every key, so the dialog's defaults are what every normal install already holds; taking them changes nothing for existing settings files. Only an incomplete file (hand-edited, imported, a minimal portable `config.ini`) behaves differently, and now consistently.

**The conflicts settled** (dialog value, then what the main window or preview used when the key was missing):

| Key | Default now | Until S06 elsewhere |
|---|---|---|
| `file/format` | 0 (PNG) | dialog 1 (JPEG); window 0 |
| `file/enabled` | true | window false |
| `file/prefix` | `screenshot.` | window empty |
| `file/target` | `<Documents>/Screenshots` (`Screenshots` in portable mode) | window empty (current directory) |
| `options/hide` | true | window false |
| `options/tray` | true | window false in three places, true in one |
| `options/message` | true | window false |
| `options/closeHide` | true | window false |
| `options/startupHide` | true | window false |
| `options/previewAutocloseTime` | 15 | preview dialog 0 |
| `options/history` | true (false in portable mode) | screenshot manager true in portable mode too |
| `actions/screen/enabled` | true | window false |
| `actions/*/hotkey` | the dialog's six hotkeys | window empty |

**How.** `Settings::value(settings, key, portable)` reads a key relative to the settings' current group (so the dialog's `beginGroup` blocks stay as they were) with the default looked up by the full key. Two defaults depend on portable mode; callers pass it (`ScreenshotManager::portableMode()`, or the manager's own flag). Not in the table: state rather than options (`position`, `geometry/*`, `lastScreenshot`, `lastUpdateCheck`), the uploader's `upload/custom/*` (defaults in `CustomUploader`, which `tools/uploader/` must keep to itself), and `options/optimizer` (absent means automatic). The prefix and folder texts keep the `OptionsDialog` translation context.

**Found on the way: `--quit` and close-to-hide.** In Qt 6, `QCoreApplication::quit()` first sends the windows a close event. With "close hides the window" on and the tray off, that close opens the "hide without a tray icon?" box, so `lightscreen --quit` waited on a dialog. With the new `closeHide=true` default the CI smoke config hit it. `--quit` now calls `QCoreApplication::exit(0)`, which leaves the event loop without closing windows; it already skipped the tray menu's confirmation, so this only removes the dialog. The tray menu's Quit is unchanged.

**Rejected.** Named C++ constants for every key (a large rename across every reader for no behaviour; the table and the `defaultsAgreeWithDialog` test catch a mistyped or missing key); rewriting the dialog's grouped reads to full keys (diff noise only).
