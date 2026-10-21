# Upload after S02 (uploader owned by the screenshot manager)

The screenshot no longer calls the uploader or the history itself; the manager does it on the screenshot's signals. Use the `lightscreen-win64` artifact of the S02 PR's CI run, with a pomf host that works set in Options > Upload (or skip steps 1 and 2 if none is at hand; step 3 still applies).

1. **Upload a capture.** Turn on "Upload" for the screenshot (or use the preview's upload button) and take a whole-screen screenshot: the tray message shows the link, and History lists the file with the link.
2. **Upload from History.** In History, right-click an older screenshot and upload it: the progress bar runs and the row gets the link.
3. **Save without upload.** Take a screenshot with upload off and history on: the file is saved and appears in History.
4. **Update check.** Options > "Check for updates now" still answers (newer version or up to date), so the updater still has a network.
