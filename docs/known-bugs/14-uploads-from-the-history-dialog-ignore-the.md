# 14. Uploads from the history dialog ignore the chosen service

- **Found:** found while writing the Stage 3 plan.
- **Where:** `dialogs/historydialog.cpp:170`, `HistoryDialog::upload`.
- **What happens:** it reads `options/upload/service` (default `"imgur"`) and passes the raw value as the service name. Every other site reads `upload/service` as an index and maps it through `Uploader::serviceName`. So an upload started from the history dialog always goes to the default service, whatever the options say.
- **Fix:** read `upload/service` and map it through `Uploader::serviceName`.
- **Platform:** any. **Test:** headless with a stub network manager. Triage: fixed in plan step M12 (one line), not a Stage 5 session.
