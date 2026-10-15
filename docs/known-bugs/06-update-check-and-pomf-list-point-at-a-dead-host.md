# 6. Update check and pomf list point at a dead host (and fail silently)

- **Found:** inferred from the code, reproduction needed.
- **Where:** `updater/updater.cpp:44` (`https://lightscreen.com.ar/version`), `widgets/pomfoptionswidget.cpp:92` (`pomf.json`), `lightscreenwindow.cpp:765`, `dialogs/updaterdialog.cpp:50`.
- **What happens:** `Updater::finished` reads the reply without checking `reply->error()`; a failed request parses as version `0` and emits `done(false)`, so "check for updates" reports "up to date" whenever the host is down. The host's state could not be verified from the cloud thread (outbound blocked). The behavior is already decided (GitHub Releases or removal; see `docs/goals.md`), so this entry is here for the "never silently succeed" part.
- **Platform:** any. **Test:** headless with a stub reply.
- **Status:** fixed in M13 (Stage 6, PR #36). The check reads GitHub Releases and emits `failed` on any network error, bad status or unreadable answer (`tests/tst_updater`); the pomf list and every other `lightscreen.com.ar` link are gone.
