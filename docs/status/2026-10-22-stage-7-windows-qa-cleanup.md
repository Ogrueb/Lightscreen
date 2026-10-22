# 2026-10-22: Stage 7, Windows QA list brought up to date

The owner asked for a review of the Windows QA list before the one pass; the owner then approved this cleanup. Editing earlier threads' QA files is an exception to the one-file-per-entry rule, made at the owner's request with no parallel PRs open; file names are kept so each check's history stays traceable.

- **Release title fixed:** `.github/workflows/release.yml` named the release "Lightscreen <version>" (and dry runs "Lightscreen <version>, <branch>"); now "Lightscreen Rd". Missed by M16. `docs/build.md` loses its "no real tag before the rename" note and points to the release notes.
- **Updated (out of date after later steps):** `imgur-removed` (service box shows "Custom"), `update-check` (fork URL `Lightscreen-Rd`, the new About links; the pomf-page step dropped), `cmake-build` (version 3.0.0.0, the Lightscreen Rd file properties), `release-dry-run` (`lightscreen-rd-3.0-win64.zip`, the optimizers and licences in the zip), `qt6-flip` (the upgrade, capture-mode and scaled-display steps moved to the newer checks that cover them), `sound-cues` ("Sound cue" label; the `Qt5Multimedia.dll` step dropped), `qscreen-capture-and-placement` (current-monitor size step left to the mixed-DPI check; real option label), `mixed-dpi-capture` and `png-optimizers` (real option labels and tab).
- **Removed:** `2026-10-21-upload-after-s02.md`, covered by `custom-uploader` (upload, history) and `update-check`.
- **Every check now uses the latest `master` artifact** instead of a given PR's.
- **New:** `2026-10-22-fresh-install.md` (first run in a portable folder: S06 defaults, and Print Screen on Windows 11, where the Snipping Tool may take the key) and `2026-10-22-release-v3.md` (the published release: title, zip, update check).
- 21 checks, as `ls docs/windows-qa` lists them. The pass stays on hold (owner).
