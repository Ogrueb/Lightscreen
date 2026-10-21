#!/usr/bin/env bash
# Include-graph check (docs/modernization-plan.md, S01 and S02):
# 1. nothing under tools/ includes anything under dialogs/ or widgets/;
# 2. tools/uploader/ includes only itself, Qt and the standard library: no other project header.
set -euo pipefail
cd "$(dirname "$0")/.."
fail=0

while IFS=: read -r file line text; do
  text="${text%$'\r'}"
  echo "::error file=$file,line=$line::tools/ must not include dialogs/ or widgets/: $text"; fail=1
done < <(grep -rnE '^[[:space:]]*#[[:space:]]*include[[:space:]]*[<"](\.\./)*(dialogs|widgets)/' tools \
           --include='*.h' --include='*.hpp' --include='*.cpp' || true)

# Project headers are included by their path from the repository root (<tools/os.h>) or, for files in the root,
# by name (<lightscreenwindow.h>). Anything that names a project directory other than tools/uploader/, or a header
# in the repository root, is out.
while IFS=: read -r file line text; do
  text="${text%$'\r'}"
  echo "::error file=$file,line=$line::tools/uploader/ must include only itself and Qt: $text"; fail=1
done < <(grep -rnE '^[[:space:]]*#[[:space:]]*include[[:space:]]*[<"]((\.\./)+|(tools|dialogs|widgets|updater)/|[a-z]+\.h[>"])' tools/uploader \
           --include='*.h' --include='*.hpp' --include='*.cpp' \
         | grep -vE '#[[:space:]]*include[[:space:]]*[<"]tools/uploader/' || true)

if [ "$fail" -eq 0 ]; then
  echo "Include graph OK"
fi
exit "$fail"
