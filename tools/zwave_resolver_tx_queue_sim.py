#!/usr/bin/env python3
"""Simulate attribute-resolver SET processing for a 232-node Z-Wave network.

Mirrors the DFS post-order walk in attribute_resolver.cpp and the 64-slot
zwave_tx queue. Writes an SVG timeline plus a text summary.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path

ZWAVE_NODE_CAPACITY = 232
MULTI_EP_NODE_COUNT = 50
MULTI_EP_COUNT = 10
TX_QUEUE_CAPACITY = 64


def endpoint_count_for_node(node_id: int) -> int:
    if node_id <= MULTI_EP_NODE_COUNT:
        return MULTI_EP_COUNT
    return 1 + ((node_id - MULTI_EP_NODE_COUNT - 1) % 5)


def build_destinations() -> list[tuple[int, int]]:
    dests: list[tuple[int, int]] = []
    for node_id in range(1, ZWAVE_NODE_CAPACITY + 1):
        for endpoint in range(endpoint_count_for_node(node_id)):
            dests.append((node_id, endpoint))
    return dests


@dataclass
class SimResult:
    destinations: list[tuple[int, int]]
    serial_max_inflight: int
    serial_max_queue: int
    burst_accepted: int
    burst_rejected: int


def simulate() -> SimResult:
    dests = build_destinations()
    # Resolver is strictly one SET in flight; TX occupancy stays 1.
    serial_max_inflight = 1
    serial_max_queue = 1
    burst_accepted = min(len(dests), TX_QUEUE_CAPACITY)
    burst_rejected = max(0, len(dests) - TX_QUEUE_CAPACITY)
    return SimResult(
        destinations=dests,
        serial_max_inflight=serial_max_inflight,
        serial_max_queue=serial_max_queue,
        burst_accepted=burst_accepted,
        burst_rejected=burst_rejected,
    )


def write_svg(result: SimResult, path: Path) -> None:
    dests = result.destinations
    total = len(dests)
    width, height = 1400, 920

    def node_x(node_id: int) -> float:
        return 80 + (node_id - 1) * (1240 / (ZWAVE_NODE_CAPACITY - 1))

    def ep_color(node_id: int) -> str:
        return "#1f6feb" if node_id <= MULTI_EP_NODE_COUNT else "#3fb950"

    lines = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}" font-family="DejaVu Sans, Arial, sans-serif">',
        '<rect width="100%" height="100%" fill="#0d1117"/>',
        '<text x="40" y="48" fill="#f0f6fc" font-size="26" font-weight="700">'
        "Z-Wave 232-node SET resolution vs zwave_tx queue</text>",
        '<text x="40" y="78" fill="#8b949e" font-size="15">'
        f"{ZWAVE_NODE_CAPACITY} nodes, {total} endpoints/SETs. "
        "Resolver: 1 in flight. zwave_tx capacity: 64 frames.</text>",
        # Pipeline boxes
        '<rect x="40" y="110" width="280" height="90" rx="12" fill="#161b22" stroke="#30363d"/>',
        '<text x="54" y="142" fill="#8b949e" font-size="13">1. Desired update</text>',
        '<text x="54" y="168" fill="#f0f6fc" font-size="16">All endpoint attrs</text>',
        '<text x="54" y="188" fill="#8b949e" font-size="13">desired != reported</text>',
        '<polygon points="330,155 360,155 360,145 390,165 360,185 360,175 330,175" fill="#58a6ff"/>',
        '<rect x="400" y="110" width="320" height="90" rx="12" fill="#161b22" stroke="#30363d"/>',
        '<text x="414" y="142" fill="#8b949e" font-size="13">2. Attribute resolver</text>',
        '<text x="414" y="168" fill="#f0f6fc" font-size="16">DFS post-order, 1 SET</text>',
        '<text x="414" y="188" fill="#8b949e" font-size="13">waits for TX complete</text>',
        '<polygon points="730,155 760,155 760,145 790,165 760,185 760,175 730,175" fill="#58a6ff"/>',
        '<rect x="800" y="110" width="280" height="90" rx="12" fill="#161b22" stroke="#30363d"/>',
        '<text x="814" y="142" fill="#8b949e" font-size="13">3. zwave_tx queue</text>',
        '<text x="814" y="168" fill="#f0f6fc" font-size="16">64 slots, QoS sort</text>',
        '<text x="814" y="188" fill="#8b949e" font-size="13">occupancy stays 1</text>',
        '<polygon points="1090,155 1120,155 1120,145 1150,165 1120,185 1120,175 1090,175" fill="#58a6ff"/>',
        '<rect x="1160" y="110" width="200" height="90" rx="12" fill="#161b22" stroke="#30363d"/>',
        '<text x="1174" y="142" fill="#8b949e" font-size="13">4. Radio</text>',
        '<text x="1174" y="168" fill="#f0f6fc" font-size="16">Send + callback</text>',
        # Network density
        '<text x="40" y="250" fill="#f0f6fc" font-size="18" font-weight="600">'
        "Network density (bar height = endpoint count)</text>",
        '<text x="40" y="274" fill="#1f6feb" font-size="13">Nodes 1-50: 10 endpoints</text>',
        '<text x="280" y="274" fill="#3fb950" font-size="13">Nodes 51-232: 1-5 endpoints</text>',
    ]

    max_ep = MULTI_EP_COUNT
    base_y = 430
    for node_id in range(1, ZWAVE_NODE_CAPACITY + 1):
        ep = endpoint_count_for_node(node_id)
        bar_h = ep * 12
        x = node_x(node_id)
        lines.append(
            f'<rect x="{x:.1f}" y="{base_y - bar_h}" width="4.2" height="{bar_h}" '
            f'fill="{ep_color(node_id)}" opacity="0.95"/>'
        )

    lines.append(
        f'<line x1="80" y1="{base_y + 4}" x2="1320" y2="{base_y + 4}" stroke="#30363d"/>'
    )
    lines.append(
        f'<text x="80" y="{base_y + 24}" fill="#8b949e" font-size="12">Node 1</text>'
    )
    lines.append(
        f'<text x="320" y="{base_y + 24}" fill="#8b949e" font-size="12">Node 50</text>'
    )
    lines.append(
        f'<text x="1240" y="{base_y + 24}" fill="#8b949e" font-size="12">Node 232</text>'
    )
    lines.append(
        f'<text x="40" y="{base_y - max_ep * 12 - 8}" fill="#8b949e" font-size="11">10 EP</text>'
    )

    # DFS order strip
    y = 500
    lines.append(
        f'<text x="40" y="{y}" fill="#f0f6fc" font-size="18" font-weight="600">'
        "DFS SET order (first 22 frames, then jump to last)</text>"
    )
    sample = dests[:22] + [dests[-1]]
    box_w = 52
    for i, (node_id, ep) in enumerate(sample):
        if i == 22:
            x = 40 + 22 * (box_w + 4) + 10
            lines.append(
                f'<text x="{x}" y="{y + 48}" fill="#8b949e" font-size="18">...</text>'
            )
            x = 40 + 23 * (box_w + 4)
        else:
            x = 40 + i * (box_w + 4)
        fill = "#1f6feb" if node_id <= MULTI_EP_NODE_COUNT else "#3fb950"
        lines.append(
            f'<rect x="{x}" y="{y + 18}" width="{box_w}" height="44" rx="6" fill="{fill}"/>'
        )
        lines.append(
            f'<text x="{x + 6}" y="{y + 36}" fill="#ffffff" font-size="11">'
            f"N{node_id}</text>"
        )
        lines.append(
            f'<text x="{x + 6}" y="{y + 52}" fill="#ffffff" font-size="11">'
            f"EP{ep}</text>"
        )

    # Queue comparison
    y = 620
    lines.append(
        f'<text x="40" y="{y}" fill="#f0f6fc" font-size="18" font-weight="600">'
        "How the 64-slot zwave_tx queue is used</text>"
    )
    # Serial bar
    lines.append(
        f'<rect x="40" y="{y + 24}" width="1200" height="36" rx="8" fill="#21262d"/>'
    )
    lines.append(
        f'<rect x="40" y="{y + 24}" width="{1200 / total}" height="36" rx="8" fill="#58a6ff"/>'
    )
    lines.append(
        f'<text x="52" y="{y + 48}" fill="#f0f6fc" font-size="14">'
        f"Resolver path: occupancy 1 / {TX_QUEUE_CAPACITY} while draining {total} SETs sequentially</text>"
    )
    lines.append(
        f'<rect x="40" y="{y + 80}" width="1200" height="36" rx="8" fill="#21262d"/>'
    )
    burst_w = 1200 * TX_QUEUE_CAPACITY / total
    lines.append(
        f'<rect x="40" y="{y + 80}" width="{burst_w:.1f}" height="36" rx="8" fill="#d29922"/>'
    )
    lines.append(
        f'<text x="52" y="{y + 104}" fill="#f0f6fc" font-size="14">'
        f"If all SETs were queued at once: {result.burst_accepted} accepted, "
        f"{result.burst_rejected} rejected (queue full)</text>"
    )

    y = 780
    lines.append(
        f'<rect x="40" y="{y}" width="1320" height="110" rx="12" fill="#161b22" stroke="#30363d"/>'
    )
    lines.append(
        f'<text x="56" y="{y + 32}" fill="#f0f6fc" font-size="16" font-weight="600">'
        "Processing rule</text>"
    )
    lines.append(
        f'<text x="56" y="{y + 58}" fill="#c9d1d9" font-size="14">'
        "GET before SET. Children before parent. One rule executes; send() enqueues one frame;"
        "</text>"
    )
    lines.append(
        f'<text x="56" y="{y + 80}" fill="#c9d1d9" font-size="14">'
        "on_resolver_send_data_complete (EXECUTION_VERIFIED) copies desired → reported, "
        "clears desired, then scans the next leaf."
        "</text>"
    )
    lines.append(
        f'<text x="56" y="{y + 102}" fill="#8b949e" font-size="13">'
        f"Totals: {ZWAVE_NODE_CAPACITY} nodes, {total} SETs, TX capacity {TX_QUEUE_CAPACITY}, "
        f"serial occupancy {result.serial_max_queue}."
        "</text>"
    )
    lines.append("</svg>")
    path.write_text("\n".join(lines), encoding="utf-8")


def write_summary(result: SimResult, path: Path) -> None:
    dests = result.destinations
    lines = [
        f"nodes={ZWAVE_NODE_CAPACITY}",
        f"endpoints={len(dests)}",
        f"node1_50_endpoints={sum(endpoint_count_for_node(n) for n in range(1, 51))}",
        f"node51_232_endpoints={sum(endpoint_count_for_node(n) for n in range(51, 233))}",
        f"first={dests[0][0]}:{dests[0][1]}",
        f"after_node1={dests[10][0]}:{dests[10][1]}",
        f"last={dests[-1][0]}:{dests[-1][1]}",
        f"serial_max_queue={result.serial_max_queue}",
        f"burst_accepted={result.burst_accepted}",
        f"burst_rejected={result.burst_rejected}",
        "order_head="
        + ",".join(f"{n}:{e}" for n, e in dests[:24]),
    ]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--svg", type=Path, required=True)
    parser.add_argument("--summary", type=Path, required=True)
    args = parser.parse_args()
    result = simulate()
    args.svg.parent.mkdir(parents=True, exist_ok=True)
    args.summary.parent.mkdir(parents=True, exist_ok=True)
    write_svg(result, args.svg)
    write_summary(result, args.summary)
    print(args.summary.read_text())


if __name__ == "__main__":
    main()
