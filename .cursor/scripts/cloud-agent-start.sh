#!/usr/bin/env bash
set -euo pipefail

docker_cmd() {
  if docker info >/dev/null 2>&1; then
    docker "$@"
  else
    sudo docker "$@"
  fi
}

if ! docker info >/dev/null 2>&1 && ! sudo docker info >/dev/null 2>&1; then
  sudo dockerd --storage-driver=fuse-overlayfs >/tmp/dockerd.log 2>&1 &
  for _ in $(seq 1 60); do
    if docker info >/dev/null 2>&1 || sudo docker info >/dev/null 2>&1; then
      break
    fi
    sleep 1
  done
fi

if ! command -v mosquitto >/dev/null 2>&1; then
  sudo DEBIAN_FRONTEND=noninteractive apt-get update -qq
  sudo DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends mosquitto
fi

if ! pgrep -x mosquitto >/dev/null; then
  mosquitto -d -p 1883
fi
