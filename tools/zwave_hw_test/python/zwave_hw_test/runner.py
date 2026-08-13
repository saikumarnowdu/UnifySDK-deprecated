"""YAML-driven requirement checks for real-hardware Z-Wave bring-up."""

from __future__ import annotations

import json
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Callable, Dict, List, Optional

from zwave_hw_test.harness import TransmitOptions, ZwaveHarness


@dataclass
class RequirementResult:
    name: str
    passed: bool
    detail: str
    duration_s: float = 0.0


def _load_spec(path: Path) -> dict:
    text = path.read_text(encoding="utf-8")
    if path.suffix in (".yaml", ".yml"):
        try:
            import yaml  # type: ignore
        except ImportError as exc:
            raise RuntimeError(
                "PyYAML required for YAML specs: pip install pyyaml"
            ) from exc
        return yaml.safe_load(text)
    return json.loads(text)


def run_requirements(
    harness: ZwaveHarness,
    spec_path: str | Path,
    poll_fn: Optional[Callable[[], None]] = None,
) -> List[RequirementResult]:
    spec = _load_spec(Path(spec_path))
    results: List[RequirementResult] = []
    defaults = spec.get("defaults", {})
    poll_timeout = float(defaults.get("poll_timeout_s", harness._poll_timeout_s))

    for step in spec.get("requirements", []):
        name = step.get("name", "unnamed")
        start = time.monotonic()
        try:
            _run_step(harness, step, poll_timeout, poll_fn)
            results.append(
                RequirementResult(
                    name=name,
                    passed=True,
                    detail="ok",
                    duration_s=time.monotonic() - start,
                )
            )
        except Exception as exc:  # noqa: BLE001 — test runner aggregates failures
            results.append(
                RequirementResult(
                    name=name,
                    passed=False,
                    detail=str(exc),
                    duration_s=time.monotonic() - start,
                )
            )
    return results


def _run_step(
    harness: ZwaveHarness,
    step: Dict[str, Any],
    poll_timeout: float,
    poll_fn: Optional[Callable[[], None]],
) -> None:
    action = step.get("action")
    if action == "get_version":
        major, minor = harness.get_version()
        expect_major = step.get("major")
        expect_minor = step.get("minor")
        if expect_major is not None and major != int(expect_major):
            raise AssertionError(f"major {major} != {expect_major}")
        if expect_minor is not None and minor != int(expect_minor):
            raise AssertionError(f"minor {minor} != {expect_minor}")
        return

    if action == "get_protocol_version":
        ver = harness.get_protocol_version_string()
        contains = step.get("contains")
        if contains and contains not in ver:
            raise AssertionError(f"{ver!r} does not contain {contains!r}")
        return

    if action == "library_type":
        lib_type = harness.get_library_type()
        expected = int(step.get("expected"))
        if lib_type != expected:
            raise AssertionError(f"library_type {lib_type} != {expected}")
        return

    if action == "send_nop":
        node_id = int(step.get("node_id"))
        tx_opts = TransmitOptions(int(step.get("tx_options", TransmitOptions.ACK)))
        harness.send_nop(node_id, tx_opts)
        timeout = float(step.get("timeout_s", poll_timeout))
        tx_status = harness.poll_until_tx_complete(timeout)
        expect = step.get("expect_tx_status")
        if expect is not None and tx_status != int(expect):
            raise AssertionError(f"tx_status {tx_status} != {expect}")
        return

    if action == "send_data":
        node_id = int(step.get("node_id"))
        payload = bytes.fromhex(step.get("hex", ""))
        tx_opts = TransmitOptions(
            int(step.get("tx_options", TransmitOptions.ACK | TransmitOptions.AUTO_ROUTE))
        )
        harness.send_data(node_id, payload, tx_opts)
        timeout = float(step.get("timeout_s", poll_timeout))
        if step.get("wait_tx"):
            tx_status = harness.poll_until_tx_complete(timeout)
            expect = step.get("expect_tx_status")
            if expect is not None and tx_status != int(expect):
                raise AssertionError(f"tx_status {tx_status} != {expect}")
        if step.get("wait_rx"):
            frame = harness.poll_until_app_frame(timeout)
            expect_hex = step.get("expect_rx_hex")
            if expect_hex and frame.data.hex() != expect_hex.replace(" ", ""):
                raise AssertionError(
                    f"rx {frame.data.hex()} != {expect_hex}"
                )
        return

    if action == "send_frame":
        func_id = int(step.get("func_id"))
        payload = bytes.fromhex(step.get("hex", ""))
        response = harness.send_frame(func_id, payload)
        expect_hex = step.get("expect_response_hex")
        if expect_hex:
            got = response.hex()
            want = expect_hex.replace(" ", "")
            if got != want:
                raise AssertionError(f"response {got} != {want}")
        return

    if action == "poll":
        deadline = time.monotonic() + float(step.get("duration_s", 1.0))
        while time.monotonic() < deadline:
            harness.poll()
            if poll_fn:
                poll_fn()
            time.sleep(0.01)
        return

    raise ValueError(f"unknown action: {action}")
