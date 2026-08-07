#!/usr/bin/env bash
# Format all first-party C++ sources with clang-format (in place).
#
# Usage:
#   scripts/format.sh          # rewrite files
#   scripts/format.sh --check  # verify only; exit non-zero if formatting differs (CI mode)
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Homebrew LLVM is keg-only, so its tools are not on PATH by default.
CLANG_FORMAT="${CLANG_FORMAT:-/opt/homebrew/opt/llvm/bin/clang-format}"

if [[ ! -x "${CLANG_FORMAT}" ]]; then
    echo "error: clang-format not found at ${CLANG_FORMAT} (brew install llvm)" >&2
    exit 1
fi

MODE_ARGS=(-i)
if [[ "${1:-}" == "--check" ]]; then
    MODE_ARGS=(--dry-run --Werror)
fi

# First-party source directories only — never third-party or generated code.
SEARCH_DIRS=()
for dir in libs server client; do
    [[ -d "${REPO_ROOT}/${dir}" ]] && SEARCH_DIRS+=("${REPO_ROOT}/${dir}")
done
if [[ ${#SEARCH_DIRS[@]} -gt 0 ]]; then
    find "${SEARCH_DIRS[@]}" \
        -type f \( -name '*.cpp' -o -name '*.h' \) -print0 |
        xargs -0 -r "${CLANG_FORMAT}" --style=file "${MODE_ARGS[@]}"
fi

echo "format.sh: done"
