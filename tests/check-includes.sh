#!/usr/bin/env bash
# Include-graph check (docs/modernization-plan.md, S01): nothing under tools/ includes anything under dialogs/ or widgets/.
# S02 adds the second rule: tools/uploader/ includes only itself and Qt.
set -euo pipefail
cd "$(dirname "$0")/.."
fail=0

while IFS=: read -r file line text; do
  text="${text%$'\r'}"
  echo "::error file=$file,line=$line::tools/ must not include dialogs/ or widgets/: $text"; fail=1
done < <(grep -rnE '^[[:space:]]*#[[:space:]]*include[[:space:]]*[<"](\.\./)*(dialogs|widgets)/' tools \
           --include='*.h' --include='*.hpp' --include='*.cpp' || true)

if [ "$fail" -eq 0 ]; then
  echo "Include graph OK"
fi
exit "$fail"
