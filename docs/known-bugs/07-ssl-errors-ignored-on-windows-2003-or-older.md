# 7. SSL errors ignored on "Windows 2003 or older"

- **Found:** inferred from the code, reproduction needed.
- **Where:** `tools/uploader/pomfuploader.cpp:39,126`, `imguruploader.cpp:113,194`, `widgets/imguroptionswidget.cpp:170`, `widgets/pomfoptionswidget.cpp:130`.
- **What happens:** `ignoreSslErrors()` is called when `QSysInfo::WindowsVersion <= WV_2003`. Harmless on supported Windows, but it is six copies of the same dead workaround and goes away with the minimum-Windows decision. Listed so Stage 5 removes it rather than ports it.
- **Platform:** n/a.
- **Status:** fixed in M03 (Stage 6). The five `ignoreSslErrors()` handlers are deleted; `PomfUploader::verify` keeps a handler that only calls `callback(false)`.
