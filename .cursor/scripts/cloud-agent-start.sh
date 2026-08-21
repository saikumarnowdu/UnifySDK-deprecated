#!/usr/bin/env bash
set -euo pipefail

if ! command -v docker >/dev/null 2>&1; then
  sudo DEBIAN_FRONTEND=noninteractive apt-get update -qq
  sudo DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends docker.io
fi

ensure_docker_running() {
  if docker info >/dev/null 2>&1 || sudo docker info >/dev/null 2>&1; then
    return 0
  fi

  # systemd is often unavailable in Cloud Agent VMs.
  sudo systemctl start docker 2>/dev/null || sudo service docker start 2>/dev/null || true
  if docker info >/dev/null 2>&1 || sudo docker info >/dev/null 2>&1; then
    return 0
  fi

  sudo mkdir -p /var/run/docker
  sudo rm -f /var/run/docker.sock
  sudo dockerd \
    --storage-driver=fuse-overlayfs \
    --iptables=false \
    >/tmp/dockerd.log 2>&1 &
  dockerd_pid=$!

  for _ in $(seq 1 120); do
    if docker info >/dev/null 2>&1 || sudo docker info >/dev/null 2>&1; then
      return 0
    fi
    if ! kill -0 "${dockerd_pid}" 2>/dev/null; then
      echo "dockerd exited unexpectedly:" >&2
      tail -30 /tmp/dockerd.log >&2 || true
      return 1
    fi
    sleep 1
  done

  echo "dockerd failed to become ready within 120s:" >&2
  tail -30 /tmp/dockerd.log >&2 || true
  return 1
}

ensure_docker_running

if ! command -v mosquitto >/dev/null 2>&1; then
  sudo DEBIAN_FRONTEND=noninteractive apt-get update -qq
  sudo DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends mosquitto
fi

if ! pgrep -x mosquitto >/dev/null; then
  mosquitto -d -p 1883
fi
