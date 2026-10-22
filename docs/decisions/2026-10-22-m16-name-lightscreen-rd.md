# 2026-10-22 (owner, M16): the fork is "Lightscreen Rd" (Redeveloped)

**Decision (owner).** The fork's name is **Lightscreen Rd**, short for "Redeveloped", written "Rd" with that exact capitalization in the UI. The analogy, from film photography: the working upstream codebase is the negative, the updated frameworks are a new developer, and the print looks like the original. The owner wanted to keep as much of the existing naming and UI as possible, so the name is the original plus a short suffix that stands for a descriptive word. The GitHub repository was renamed from `Ogrueb/Lightscreen` to `Ogrueb/Lightscreen-Rd` by the owner before the first release, so no shipped build refers to the old name (GitHub redirects the old one).

Considered and not chosen: "Lightscreen Qt6" (names an implementation detail that will date, and reads as upstream), NG (suggests a redesign), CE (Community Edition; the runner-up), Remastered, FX ("Fixed"), Open Shutter, Rolling Shutter (a capture defect), RE/ME (ambiguous).

**What changes (table approved by the owner).**

| Where | Now |
|---|---|
| Window titles and tray tooltip | "Lightscreen Rd" (main window, options, naming, updater, area mode, window picker, message boxes, upload progress) |
| About tab | "Lightscreen Rd (Redeveloped) is a maintained fork of Lightscreen…", credit to Christian Kaiser, links to the fork and to the original |
| Windows file properties | product "Lightscreen Rd", company "Lightscreen Rd contributors", description "Lightscreen Rd Screenshot Tool" |
| Update check | `Ogrueb/Lightscreen-Rd`; User-Agent `Lightscreen-Rd/<version>` |
| Release zip | `lightscreen-rd-<version>-win64.zip` |
| Application display name | "Lightscreen Rd" (`QGuiApplication::applicationDisplayName`) |

**What stays.** `lightscreen.exe`; the default file prefix for saved screenshots (`screenshot.`, unchanged since upstream; owner, 2026-10-22); CI's `lightscreen-win64` artifact name; the organization and application names `K` / `Lightscreen`, which locate the settings and history and key the single-instance check (now set in `Settings::applyApplicationIdentity`, with a test); the start-at-login entry named "Lightscreen" (registry value and `.desktop` file), so an upgrade reuses it; portable `config.ini`; sentences that mention the app by name ("Lightscreen will keep running…") and labels such as "Lightscreen Settings (*.ini)"; the CMake project name (the release workflow reads it).

**Choices made in M16, for the owner to override.**
- **Version 3.0** in `CMakeLists.txt`, so the first release can be tagged `v3.0.0` (Stage 7). The release workflow refuses a tag that does not match.
- **Copyright line for the fork's own code:** "Copyright (C) 2026 Lightscreen Rd contributors" on the files written in the fork (`tools/capturegeometry`, `tools/optimizer`, `tools/settings`) and in the Windows file properties next to Christian Kaiser's line. This settles what `2026-10-15-m14-no-license-header-on-tests.md` left for M16; test sources stay without a header.
