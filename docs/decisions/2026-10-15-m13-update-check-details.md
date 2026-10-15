# 2026-10-15: M13, details of the update check the plan left open

**Decisions.**
- A tag that does not start with a version number (after dropping `v`) is `failed`, like a body without `tag_name`; a suffix after the number (`2.6.0-beta`) is ignored. Pre-releases and drafts never reach the check, because GitHub's `releases/latest` skips them.
- HTTP 404 gets its own message ("No release has been published yet at <releases page>"), since that is what GitHub answers until the fork's first release. Other errors show Qt's error string; a non-200 status with no network error shows the status.
- A second `check()` while one is running sends nothing. Destroying the updater mid-request aborts the request without emitting anything (the "Check now" dialog closed early).
- The about tab links the fork's GitHub page (built from `UPDATE_REPO`, so M16 needs no `.ui` edit) and the original Lightscreen repository as credit. The pomf "View Help" link is removed rather than repointed: there is no fork help page, and S03 rewrites that widget. The naming dialog's date `[?]` points at Qt's date format docs, which is what the format string is.

**Rejected.** Comparing tags as strings (2.10 < 2.9); treating an unreadable tag as "up to date" (that is bug 6's silent success); asking the Atom feed or scraping the releases page instead of the API (no stable format).
