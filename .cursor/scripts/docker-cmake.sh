#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${repo_root}"

"${repo_root}/.cursor/scripts/cloud-agent-start.sh"

docker_cmd() {
  if docker info >/dev/null 2>&1; then
    docker "$@"
  else
    sudo docker "$@"
  fi
}

image_tag="uic_amd64"

if [[ $# -eq 0 ]]; then
  set -- bash
fi

docker_cmd run --rm -it --network host \
  -v "${repo_root}:${repo_root}" \
  -w "${repo_root}" \
  "${image_tag}" \
  "$@"
