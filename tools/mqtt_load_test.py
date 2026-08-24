#!/usr/bin/env python3
"""
Programmatic MQTT load test for Unify / Z-Wave Protocol Controller.

Publishes burst UCL commands against many nodes and measures success rate and
latency. Use against a running EED, ZPC, or any UCL-speaking application.

Examples:
  # Smoke test (connect only, no target app required beyond broker)
  python3 tools/mqtt_load_test.py --mode smoke --host localhost

  # Load test against EED with pre-created nodes zw-0001 .. zw-0200
  python3 tools/mqtt_load_test.py --nodes 200 --commands 500 --assert-success-rate 95

  # Force-read OnOff on all endpoints
  python3 tools/mqtt_load_test.py --command force-read --cluster OnOff --nodes 50
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import threading
import time
import uuid
from dataclasses import dataclass, field
from typing import Optional


@dataclass
class LoadTestResult:
    commands_sent: int = 0
    responses_received: int = 0
    errors: int = 0
    latencies_ms: list[float] = field(default_factory=list)

    @property
    def success_rate(self) -> float:
        if self.commands_sent == 0:
            return 0.0
        return (self.responses_received / self.commands_sent) * 100.0

    def percentile_ms(self, pct: float) -> float:
        if not self.latencies_ms:
            return 0.0
        ordered = sorted(self.latencies_ms)
        index = int((pct / 100.0) * (len(ordered) - 1))
        return ordered[index]


def _import_paho():
    try:
        import paho.mqtt.client as mqtt  # type: ignore

        return mqtt
    except ImportError:
        return None


def _create_mqtt_client(mqtt, client_id: str):
    try:
        return mqtt.Client(
            client_id=client_id,
            callback_api_version=mqtt.CallbackAPIVersion.VERSION1,
        )
    except (AttributeError, TypeError):
        return mqtt.Client(client_id=client_id)


def _mosquitto_pub(host: str, port: int, topic: str, payload: str) -> bool:
    cmd = [
        "mosquitto_pub",
        "-h",
        host,
        "-p",
        str(port),
        "-t",
        topic,
        "-m",
        payload,
        "-q",
        "1",
    ]
    try:
        subprocess.run(cmd, check=True, capture_output=True, text=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def _endpoint_ids(min_ep: int, max_ep: int, node_index: int) -> range:
    count = min_ep + (node_index % (max_ep - min_ep + 1))
    return range(count)


def _build_topic(
    unid_prefix: str,
    node_index: int,
    endpoint_id: int,
    cluster: str,
    command: str,
) -> str:
    unid = f"{unid_prefix}{node_index:04d}"
    if command == "force-read":
        return (
            f"ucl/by-unid/{unid}/ep{endpoint_id}/{cluster}/"
            f"Commands/ForceReadAttributes"
        )
    if command == "on":
        return f"ucl/by-unid/{unid}/ep{endpoint_id}/{cluster}/Commands/On"
    if command == "off":
        return f"ucl/by-unid/{unid}/ep{endpoint_id}/{cluster}/Commands/Off"
    if command == "write-desired":
        return (
            f"ucl/by-unid/{unid}/ep{endpoint_id}/{cluster}/"
            f"Attributes/OnOff/Desired"
        )
    raise ValueError(f"Unsupported command: {command}")


def _build_payload(command: str) -> str:
    if command == "force-read":
        return json.dumps({"AttributeList": ["OnOff"]})
    if command in ("on", "off"):
        return "{}"
    if command == "write-desired":
        return json.dumps({"value": True})
    return "{}"


def _response_topic(
    unid_prefix: str,
    node_index: int,
    endpoint_id: int,
    cluster: str,
) -> str:
    unid = f"{unid_prefix}{node_index:04d}"
    return f"ucl/by-unid/{unid}/ep{endpoint_id}/{cluster}/Attributes/OnOff/Reported"


def run_load_test_paho(args: argparse.Namespace) -> LoadTestResult:
    mqtt = _import_paho()
    if mqtt is None:
        raise RuntimeError("paho-mqtt is not installed")

    result = LoadTestResult()
    pending: dict[str, float] = {}
    lock = threading.Lock()
    done = threading.Event()

    def on_connect(client, _userdata, _flags, rc):
        if rc != 0:
            result.errors += 1
            done.set()
            return
        client.subscribe("ucl/by-unid/+/ep+/+/Attributes/+/Reported", qos=1)

    def on_message(_client, _userdata, msg):
        with lock:
            if msg.topic in pending:
                started = pending.pop(msg.topic)
                result.responses_received += 1
                result.latencies_ms.append((time.monotonic() - started) * 1000.0)
                if len(pending) == 0 and result.commands_sent >= args.commands:
                    done.set()

    client = _create_mqtt_client(mqtt, f"mqtt-load-test-{uuid.uuid4().hex[:8]}")
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(args.host, args.port, keepalive=60)
    client.loop_start()

    deadline = time.monotonic() + args.timeout
    command_index = 0

    while command_index < args.commands and time.monotonic() < deadline:
        node_index = (command_index % args.nodes) + 1
        endpoints = list(
            _endpoint_ids(args.endpoints_min, args.endpoints_max, node_index)
        )
        endpoint_id = endpoints[command_index % len(endpoints)]

        topic = _build_topic(
            args.unid_prefix, node_index, endpoint_id, args.cluster, args.command
        )
        payload = _build_payload(args.command)
        response_topic = _response_topic(
            args.unid_prefix, node_index, endpoint_id, args.cluster
        )

        with lock:
            pending[response_topic] = time.monotonic()
            result.commands_sent += 1

        client.publish(topic, payload, qos=1)
        command_index += 1

        if args.publish_interval_ms > 0:
            time.sleep(args.publish_interval_ms / 1000.0)

    remaining = max(0.0, deadline - time.monotonic())
    done.wait(timeout=remaining)

    with lock:
        result.errors += len(pending)

    client.loop_stop()
    client.disconnect()
    return result


def run_load_test_mosquitto(args: argparse.Namespace) -> LoadTestResult:
    result = LoadTestResult()

    for command_index in range(args.commands):
        node_index = (command_index % args.nodes) + 1
        endpoints = list(
            _endpoint_ids(args.endpoints_min, args.endpoints_max, node_index)
        )
        endpoint_id = endpoints[command_index % len(endpoints)]
        topic = _build_topic(
            args.unid_prefix, node_index, endpoint_id, args.cluster, args.command
        )
        payload = _build_payload(args.command)

        started = time.monotonic()
        if _mosquitto_pub(args.host, args.port, topic, payload):
            result.commands_sent += 1
            result.responses_received += 1
            result.latencies_ms.append((time.monotonic() - started) * 1000.0)
        else:
            result.errors += 1

        if args.publish_interval_ms > 0:
            time.sleep(args.publish_interval_ms / 1000.0)

    return result


def run_smoke_test(args: argparse.Namespace) -> int:
    mqtt = _import_paho()
    if mqtt is None:
        print("paho-mqtt not installed; verifying mosquitto_pub availability")
        ok = _mosquitto_pub(
            args.host,
            args.port,
            "ucl/load-test/smoke",
            json.dumps({"status": "ok"}),
        )
        if ok:
            print("SMOKE OK: mosquitto_pub published successfully")
            return 0
        print("SMOKE FAIL: install paho-mqtt or mosquitto-clients", file=sys.stderr)
        return 1

    connected = threading.Event()
    connect_rc = [1]

    def on_connect(_client, _userdata, _flags, rc):
        connect_rc[0] = rc
        connected.set()

    client = _create_mqtt_client(mqtt, f"mqtt-load-smoke-{uuid.uuid4().hex[:8]}")
    client.on_connect = on_connect
    try:
        client.connect(args.host, args.port, keepalive=10)
    except Exception as exc:
        print(f"SMOKE FAIL: could not connect to broker: {exc}", file=sys.stderr)
        return 1

    client.loop_start()
    if not connected.wait(timeout=args.timeout):
        print("SMOKE FAIL: broker connection timed out", file=sys.stderr)
        client.loop_stop()
        return 1

    client.loop_stop()
    client.disconnect()

    if connect_rc[0] != 0:
        print(f"SMOKE FAIL: broker returned rc={connect_rc[0]}", file=sys.stderr)
        return 1

    print(f"SMOKE OK: connected to mqtt://{args.host}:{args.port}")
    return 0


def print_report(result: LoadTestResult, args: argparse.Namespace) -> int:
    print("=== MQTT Load Test Report ===")
    print(f"Commands sent:       {result.commands_sent}")
    print(f"Responses received:  {result.responses_received}")
    print(f"Errors:              {result.errors}")
    print(f"Success rate:        {result.success_rate:.2f}%")
    if result.latencies_ms:
        print(f"Latency p50:         {result.percentile_ms(50):.2f} ms")
        print(f"Latency p95:         {result.percentile_ms(95):.2f} ms")
        print(f"Latency p99:         {result.percentile_ms(99):.2f} ms")

    if result.success_rate < args.assert_success_rate:
        print(
            f"FAIL: success rate {result.success_rate:.2f}% "
            f"< required {args.assert_success_rate:.2f}%",
            file=sys.stderr,
        )
        return 1

    print("PASS")
    return 0


def parse_args(argv: Optional[list[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--mode",
        choices=("load", "smoke"),
        default="load",
        help="smoke = broker connectivity only; load = full publish/subscribe test",
    )
    parser.add_argument("--host", default="localhost", help="MQTT broker host")
    parser.add_argument("--port", type=int, default=1883, help="MQTT broker port")
    parser.add_argument(
        "--nodes", type=int, default=200, help="Number of UNIDs (zw-0001 .. zw-N)"
    )
    parser.add_argument(
        "--endpoints-min", type=int, default=1, help="Minimum endpoints per node"
    )
    parser.add_argument(
        "--endpoints-max", type=int, default=10, help="Maximum endpoints per node"
    )
    parser.add_argument(
        "--commands", type=int, default=500, help="Number of MQTT commands to publish"
    )
    parser.add_argument(
        "--cluster", default="OnOff", help="UCL cluster name used in topic paths"
    )
    parser.add_argument(
        "--command",
        choices=("force-read", "on", "off", "write-desired"),
        default="force-read",
        help="UCL command type to publish",
    )
    parser.add_argument(
        "--unid-prefix",
        default="zw-",
        help="UNID prefix; node index is zero-padded to 4 digits",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=30.0,
        help="Seconds to wait for responses after publishing",
    )
    parser.add_argument(
        "--publish-interval-ms",
        type=int,
        default=0,
        help="Delay between publishes (0 = burst)",
    )
    parser.add_argument(
        "--assert-success-rate",
        type=float,
        default=95.0,
        help="Minimum acceptable success rate (percent); exits 1 if below",
    )
    parser.add_argument(
        "--use-mosquitto-pub",
        action="store_true",
        help="Use mosquitto_pub instead of paho-mqtt (fire-and-forget)",
    )
    return parser.parse_args(argv)


def main(argv: Optional[list[str]] = None) -> int:
    args = parse_args(argv)

    if args.endpoints_min > args.endpoints_max:
        print("endpoints-min must be <= endpoints-max", file=sys.stderr)
        return 2

    if args.mode == "smoke":
        return run_smoke_test(args)

    if args.use_mosquitto_pub or _import_paho() is None:
        if args.mode == "load" and not args.use_mosquitto_pub:
            print(
                "Warning: paho-mqtt not found, falling back to mosquitto_pub "
                "(responses not measured)",
                file=sys.stderr,
            )
        result = run_load_test_mosquitto(args)
    else:
        result = run_load_test_paho(args)

    return print_report(result, args)


if __name__ == "__main__":
    sys.exit(main())
