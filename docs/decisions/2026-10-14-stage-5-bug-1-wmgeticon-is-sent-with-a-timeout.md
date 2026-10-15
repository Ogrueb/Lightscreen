# 2026-10-14 (Stage 5, bug 1): `WM_GETICON` is sent with a timeout

- **The window picker's last icon fallback uses `SendMessageTimeout(..., SMTO_ABORTIFHUNG, 100 ms)`,** not the plain `SendMessage` the triage table names. The picker asks on every mouse move, and a plain `SendMessage` to a hung window would freeze the picker until that window answers. A window that times out simply shows no icon. *Rejected:* plain `SendMessage` (can block indefinitely).
