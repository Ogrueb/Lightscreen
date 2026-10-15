# 2026-10-14 (M01): details of the CI workflow

The plan left these open; each is the simplest option that fits the decisions above.

- **The smoke config also sets `options/disableUpdater=true`.** The app schedules an update check five seconds after start, against `lightscreen.com.ar`. The smoke quits after two seconds so it would not fire, but the setting makes the "no calls to lightscreen.com.ar" rule hold by construction rather than by timing. *Rejected:* relying on the timing.
- **The smoke script lives inline in `ci.yml`,** written to `build/smoke.sh` at run time. The plan names only the workflow file, and the script is 30 lines. *Rejected:* a separate `scripts/` file (a new top-level directory for one script; revisit if M02 or later steps want to call it).
- **Qt on Windows comes from `jurplel/install-qt-action@v4`** with its built-in cache, MSVC from `ilammy/msvc-dev-cmd@v1`. Actions are pinned by major tag, as the plan says. *Rejected:* calling `aqtinstall` directly (more steps, and caching would be by hand).
- **The Windows artifact is the deploy folder as uploaded by `actions/upload-artifact`,** which zips it. *Rejected:* zipping it first (the download would be a zip inside a zip).
- **Push runs are limited to `master`;** branches get CI through their pull request. Running on every push ran each PR twice. The workflow also runs on `workflow_dispatch` and cancels superseded runs on the same ref.
- **`actions/checkout` and `actions/upload-artifact` are on v5** (Node 24), since v4 triggers GitHub's Node 20 deprecation warning. `ilammy/msvc-dev-cmd@v1` still warns; it has no newer major.
