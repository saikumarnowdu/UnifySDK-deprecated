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

if ! command -v git-lfs >/dev/null 2>&1; then
  sudo DEBIAN_FRONTEND=noninteractive apt-get update -qq
  sudo DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends git-lfs
fi

git lfs install --local >/dev/null 2>&1 || true
git lfs pull

image_tag="uic_amd64"
arch="amd64"
uid="$(id -u)"
gid="$(id -g)"

if ! docker_cmd image inspect "${image_tag}" >/dev/null 2>&1; then
  docker_cmd build --network host \
    -t "${image_tag}" \
    --build-arg "ARCH=${arch}" \
    --build-arg "USER_ID=${uid}" \
    --build-arg "GROUP_ID=${gid}" \
    ./docker
fi

docker_cmd run --rm --network host \
  -v "${repo_root}:${repo_root}" \
  -w "${repo_root}" \
  "${image_tag}" \
  bash -lc '
set -euo pipefail
export HEADLESS_HOST=true
mkdir -p build-docker
cmake -B build-docker -GNinja \
  -DBUILD_TESTING=ON \
  -DBUILD_DEV_GUI=OFF \
  -DBUILD_IMAGE_PROVIDER=OFF \
  -DBUILD_MATTER_BRIDGE=OFF
cmake --build build-docker -j"$(nproc)"
'
