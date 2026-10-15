# 2026-10-14 (Stage 5, bug 13): what `updateHistory` does with a row that already has a URL

- **The newer upload overwrites the URL and delete URL** of every row for that file name, and refreshes the time. Re-uploading means the user wants the new link; the old delete link is lost from history, as it was for files never recorded. *Rejected:* keeping the first URL (the history would show a link the user replaced); a new row per upload (that is the bug).
- **An empty delete hash stores an empty delete URL,** matching `saveHistory`, instead of the bare `https://imgur.com/delete/` the update branch used to write. *Rejected:* keeping the bare prefix (a dead link in the history dialog for uploaders without a delete hash, such as pomf).
