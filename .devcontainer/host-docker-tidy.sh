#!/usr/bin/env bash
# Runs on the HOST via devcontainer.json "initializeCommand" (before the dev container is created).
# Keeps this repo from accumulating duplicate stopped containers / untagged images from this compose app only.
# Does NOT run "docker compose down" (Dev Containers manages stop/start) and does NOT remove volumes.

set -euo pipefail
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${REPO_ROOT}"

if ! command -v docker >/dev/null 2>&1; then
    exit 0
fi

COMPOSE=(docker compose -f "${REPO_ROOT}/docker-compose.yml" --project-name bluesim)

# Stopped containers for this compose project only (does not stop a running dev container).
"${COMPOSE[@]}" rm -f 2>/dev/null || true

if docker container prune --help 2>/dev/null | grep -q -- '--filter'; then
    docker container prune -f --filter "label=com.docker.compose.project=bluesim" 2>/dev/null || true
fi

# Dangling images created for this compose project (typical after "Rebuild without cache" retags latest).
if docker image prune --help 2>/dev/null | grep -q -- '--filter'; then
    docker image prune -f --filter "label=com.docker.compose.project=bluesim" 2>/dev/null || true
fi
