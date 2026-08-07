#!/usr/bin/env bash
# Stop the dockerized Modulo development database.
#
# Usage:
#   scripts/db-down.sh          # stop; data volume is preserved
#   scripts/db-down.sh --wipe   # stop AND delete all database data
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

DOWN_ARGS=()
if [[ "${1:-}" == "--wipe" ]]; then
    read -r -p "This DELETES all data in the dev and test databases. Continue? [y/N] " reply
    [[ "${reply}" == "y" || "${reply}" == "Y" ]] || { echo "aborted"; exit 1; }
    DOWN_ARGS=(--volumes)
fi

docker compose -f "${REPO_ROOT}/docker/docker-compose.yml" down "${DOWN_ARGS[@]+"${DOWN_ARGS[@]}"}"

echo "db-down.sh: done"
