#!/usr/bin/env bash
#
# lint.sh — run the same C++ code-style checks locally that CI enforces:
#   * clang-format over all C/C++ sources (src/ + tests/)
#   * clang-tidy over the SDK-free updater logic
#
# This mirrors .github/workflows/lint.yml so you can catch (and auto-fix)
# problems before pushing.
#
# Usage:
#   scripts/lint.sh            # check only; non-zero exit if anything is off
#   scripts/lint.sh --fix      # rewrite files in place to satisfy the rules
#
# Requires clang-format, clang-tidy and cmake on PATH. Set CLANG_SUFFIX to pin a
# version, e.g.  CLANG_SUFFIX=-18 scripts/lint.sh

set -euo pipefail

cd "$(dirname "$0")/.."

SUFFIX="${CLANG_SUFFIX:-}"
CLANG_FORMAT="clang-format${SUFFIX}"
CLANG_TIDY="clang-tidy${SUFFIX}"
FIX=0
[ "${1:-}" = "--fix" ] && FIX=1

# Collect the C/C++ sources clang-format owns (skip the plist template).
mapfile -t FILES < <(find src tests \
  \( -name '*.cpp' -o -name '*.h' \) ! -name '*.plist.in' | sort)

echo "==> clang-format (${#FILES[@]} files)"
if [ "$FIX" -eq 1 ]; then
  "$CLANG_FORMAT" -i "${FILES[@]}"
  echo "    formatted in place"
else
  "$CLANG_FORMAT" --dry-run --Werror "${FILES[@]}"
  echo "    OK"
fi

# clang-tidy needs a compile database describing how each TU is built.
echo "==> configuring compile database (SDK-free)"
cmake -S . -B build-lint \
  -DCMAKE_BUILD_TYPE=Debug \
  -DVW_BUILD_PLUGIN=OFF \
  -DVW_BUILD_TESTS=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON >/dev/null

echo "==> clang-tidy (SDK-free updater logic)"
TIDY_ARGS=(-p build-lint --warnings-as-errors='*')
[ "$FIX" -eq 1 ] && TIDY_ARGS+=(--fix --fix-errors)
"$CLANG_TIDY" "${TIDY_ARGS[@]}" src/UpdaterFlow.cpp
echo "    OK"

echo "All lint checks passed."
