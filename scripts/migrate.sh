#!/usr/bin/env bash
# Apply pending database migrations using the modulo_migrate binary.
#
# The database URL comes from, in order: an existing MODULO_DB_URL in the
# environment, the repo-root .env file, or the dev-database default.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ -z "${MODULO_DB_URL:-}" && -f "${REPO_ROOT}/.env" ]]; then
    set -a
    # shellcheck source=/dev/null
    source "${REPO_ROOT}/.env"
    set +a
fi
export MODULO_DB_URL="${MODULO_DB_URL:-postgresql://modulo:modulo@localhost:5433/modulo_dev}"

MIGRATE_BIN="${REPO_ROOT}/build/dev/server/migrate/modulo_migrate"
if [[ ! -x "${MIGRATE_BIN}" ]]; then
    echo "error: ${MIGRATE_BIN} not found — build it first:" >&2
    echo "  cmake --preset dev && cmake --build --preset dev" >&2
    exit 1
fi

exec "${MIGRATE_BIN}" --dir "${REPO_ROOT}/db/migrations"
