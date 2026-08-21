#!/usr/bin/env bash
set -euo pipefail

if ! command -v docker >/dev/null 2>&1; then
  sudo DEBIAN_FRONTEND=noninteractive apt-get update -qq
  sudo DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends docker.io
fi

docker_ready() {
  docker info >/dev/null 2>&1 || sudo docker info >/dev/null 2>&1
}

start_dockerd_manual() {
  local driver=$1
  local data_root="/tmp/cursor-docker-data"

  sudo mkdir -p /var/run/docker "${data_root}"
  sudo rm -f /var/run/docker.sock

  sudo dockerd \
    --data-root="${data_root}" \
    --storage-driver="${driver}" \
    --iptables=false \
    >/tmp/dockerd.log 2>&1 &
  dockerd_pid=$!

  for _ in $(seq 1 60); do
    if docker_ready; then
      return 0
    fi
    if ! kill -0 "${dockerd_pid}" 2>/dev/null; then
      return 1
    fi
    sleep 1
  done

  sudo kill "${dockerd_pid}" 2>/dev/null || true
  wait "${dockerd_pid}" 2>/dev/null || true
  return 1
}

ensure_docker_running() {
  if docker_ready; then
    return 0
  fi

  # systemd is often unavailable in Cloud Agent VMs.
  sudo systemctl start docker 2>/dev/null || sudo service docker start 2>/dev/null || true
  if docker_ready; then
    return 0
  fi

  for driver in vfs fuse-overlayfs overlay2; do
    if start_dockerd_manual "${driver}"; then
      return 0
    fi
  done

  echo "Warning: dockerd failed to start; install will use native build fallback" >&2
  tail -30 /tmp/dockerd.log >&2 || true
  return 1
}

ensure_docker_running || true

if ! command -v mosquitto >/dev/null 2>&1; then
  sudo DEBIAN_FRONTEND=noninteractive apt-get update -qq
  sudo DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends mosquitto
fi

if ! pgrep -x mosquitto >/dev/null; then
  mosquitto -d -p 1883
fi
