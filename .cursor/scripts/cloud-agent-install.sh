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

build_native() {
  echo "Using native amd64 setup (Docker unavailable in this VM)" >&2
  sudo DEBIAN_FRONTEND=noninteractive apt-get update -qq
  sudo DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build pkg-config git-lfs \
    nlohmann-json3-dev protobuf-compiler libprotobuf-dev python3-pip \
    libmbedtls-dev libssl-dev libmosquitto-dev \
    libboost-system-dev libboost-filesystem-dev libboost-log-dev \
    libboost-program-options-dev libboost-thread-dev \
    libjsoncpp-dev libyaml-cpp-dev libedit-dev libreadline-dev \
    libasound2 libgbm1 xauth xvfb

  export HEADLESS_HOST=true
  build_dir="build"
  cmake -B "${build_dir}" -GNinja \
    -DBUILD_TESTING=ON \
    -DBUILD_DEV_GUI=OFF \
    -DBUILD_IMAGE_PROVIDER=OFF \
    -DBUILD_MATTER_BRIDGE=OFF \
    -DBUILD_ZIGPC=OFF \
    -DBUILD_ZPC=OFF \
    -DBUILD_UIC_DEMO=OFF \
    -DBUILD_UPVL=OFF \
    -DBUILD_GMS=OFF \
    -DBUILD_AOXPC=OFF \
    -DBUILD_POSITIONING=OFF \
    -DBUILD_NAL=OFF \
    -DBUILD_UPTI_CAP=OFF \
    -DBUILD_UPTI_WRITER=OFF \
    -DBUILD_CPCD=OFF \
    -DBUILD_ZIGBEED=OFF \
    -DBUILD_OTBR=OFF \
    -DBUILD_EPC=OFF
  echo "Native toolchain configured; full arm64 build requires Docker" >&2
}

build_arm64_docker() {
  local image_tag="uic_arm64"
  local arch="arm64"
  local build_dir="build-arm64"
  local uid gid
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
}

if docker_cmd info >/dev/null 2>&1; then
  build_arm64_docker
else
  build_native
fi
