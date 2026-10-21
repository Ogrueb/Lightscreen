# Custom uploader replaces pomf (PR #N, bug 22)

Use the `lightscreen-win64` artifact of the S03 PR's CI run (or of `master` after it merged) and an upload host you trust. Any host that takes a multipart POST and answers JSON works; a pomf clone takes the defaults (field `files[]`, link path `files.0.url`, URL ending in `/upload.php`).

1. **Migration.** If your settings had a pomf URL, open Options > Upload: "Upload URL" shows it with `/upload.php` added, field `files[]`, link path `files.0.url`.
2. **Test upload.** Fill in the host's values (or keep the migrated ones) and press "Test upload": a link appears next to the button and opens a 1x1 image in the browser. With a wrong link path (for example `nothing`), the button shows "No link in the response: "nothing" not found".
3. **Real upload.** Save, take a screenshot with "Upload" on: the tray message shows the link, the link opens the screenshot, and History lists it with the link.
4. **Cancel.** Start an upload of a large screenshot and choose "Cancel upload" in the main window's upload menu: no error message appears.
