#!/usr/bin/env python3
"""Run real-hardware Z-Wave requirement specs against an NCP (no UCL/MQTT)."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

# Allow running without pip install: add python/ to path.
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "python"))

from zwave_hw_test.harness import ZwaveHarness
from zwave_hw_test.runner import run_requirements


def main() -> int:
    parser = argparse.ArgumentParser(description="Z-Wave hardware requirement runner")
    parser.add_argument(
        "--port",
        required=True,
        help="Serial port for Z-Wave NCP (e.g. /dev/ttyUSB0)",
    )
    parser.add_argument(
        "--spec",
        type=Path,
        default=ROOT / "python/zwave_hw_test/requirements/serial_baseline.yaml",
        help="YAML/JSON requirement spec",
    )
    parser.add_argument("--log-level", type=int, default=2, help="0=debug .. 4=critical")
    parser.add_argument("--poll-timeout", type=float, default=30.0)
    args = parser.parse_args()

    with ZwaveHarness(args.port, poll_timeout_s=args.poll_timeout, log_level=args.log_level) as h:
        results = run_requirements(h, args.spec)

    failed = 0
    for r in results:
        status = "PASS" if r.passed else "FAIL"
        print(f"[{status}] {r.name} ({r.duration_s:.2f}s) — {r.detail}")
        if not r.passed:
            failed += 1

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
