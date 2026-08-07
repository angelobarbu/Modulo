#!/usr/bin/env bash
# Start the dockerized Modulo development database (Postgres 16 on localhost:5433)
# and wait until it is healthy.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if ! docker info > /dev/null 2>&1; then
    echo "error: Docker is not running (open -a Docker)" >&2
    exit 1
fi

docker compose -f "${REPO_ROOT}/docker/docker-compose.yml" up -d --wait

echo "db-up.sh: postgres ready on localhost:5433 (databases: modulo_dev, modulo_test)"
