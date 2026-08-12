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

image_tag="uic_arm64"
arch="arm64"
build_dir="build-arm64"
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

# Enable QEMU user-mode emulation so arm64 unit tests can run on the host.
docker_cmd run --user 0 --privileged --rm "${image_tag}" update-binfmts --enable >/dev/null 2>&1 || true

docker_cmd run --rm --network host \
  -v "${repo_root}:${repo_root}" \
  -w "${repo_root}" \
  "${image_tag}" \
  bash -lc "
set -euo pipefail
export HEADLESS_HOST=true
mkdir -p ${build_dir}
cmake -B ${build_dir} -GNinja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm64_debian.cmake \
  -DBUILD_TESTING=ON \
  -DBUILD_DEV_GUI=OFF \
  -DBUILD_IMAGE_PROVIDER=OFF \
  -DBUILD_MATTER_BRIDGE=OFF
cmake --build ${build_dir} -j\"\$(nproc)\"
"
