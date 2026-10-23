# Custom uploader replaces pomf (PR #50, bug 22)

Use the `lightscreen-win64` artifact of the latest `master` CI run and an upload host you trust. Any host that takes a multipart POST and answers JSON works; a pomf clone takes the defaults (field `files[]`, link path `files.0.url`, URL ending in `/upload.php`).

1. **Migration.** If your settings had a pomf URL, open Options > Upload: "Upload URL" shows it with `/upload.php` added, field `files[]`, link path `files.0.url`.
2. **Test upload.** Fill in the host's values (or keep the migrated ones) and press "Test upload": a link appears next to the button and opens a 1x1 image in the browser. With a wrong link path (for example `nothing`), the button shows "No link in the response: "nothing" not found".
3. **Real upload.** Save, take a screenshot with "Upload" on: the tray message shows the link, the link opens the screenshot, and History lists it with the link.
4. **Cancel.** Start an upload of a large screenshot and choose "Cancel upload" in the main window's upload menu: no error message appears.
5. **History menu** (from `2026-10-15-imgur-removed.md`, bug 14). Open View History. A screenshot that was never uploaded has an empty URL cell. Right-click that empty cell: "Copy URL" and "Open delete link" are greyed out. (Right-clicking the file name shows "Copy Path" and "Open Location" instead and never lists "Open delete link".) Right-click the link of the upload from step 3: "Copy URL" works, and "Open delete link" is greyed out unless the host returned a delete link and its path is set in Options > Upload.
