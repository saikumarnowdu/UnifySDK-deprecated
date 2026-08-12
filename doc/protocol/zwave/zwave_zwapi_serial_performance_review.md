# Z-Wave / zwapi Serial API Performance Review

Review of `applications/zpc` **zwave** and **zwapi** (`zwave_api`) focused on
host↔NCP Serial API bandwidth utilization and high-node-count throughput.
Source baseline: Unify SDK **`ver_1.6.0`** (last in-repo ZPC tree; removed in
`ver_1.7.0` / standalone [z-wave-protocol-controller](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller)).

Concrete fixes from this review are packaged as:

`tools/zpc_serial_api_perf/0001-zwave-zwapi-serial-api-performance.patch`

Apply against a `ver_1.6.0` (or equivalent) ZPC tree; see
`tools/zpc_serial_api_perf/README.md`.

---

## Executive verdict

**UART bit rate (115200) is not the primary bottleneck.** The stack is a classic
INS12350 stop-and-wait Serial API: one host REQ at a time (ACK + optional RES),
one RF `ZW_SendData` in flight, and a Contiki TX state machine that often sits
idle for multi-second response back-offs. Under load, useful Serial API
bandwidth is lost to:

1. Dropping queued frames on transient transport `BUSY` (forces upper-layer retries)
2. Always enabling `TRANSMIT_OPTION_EXPLORE` for non-fasttrack TX (long RF occupancy)
3. Busy-spin ACK waits, 20 retries, and silent-port timeout bugs in zwapi session
4. Global TX lock while waiting for replies from a single NodeID

---

## Architecture (relevant path)

```
App / CC / Attribute Resolver
        │
        ▼
 zwave_tx (QoS heap ≤64) ── Contiki: ONE current_tx_session_id
        │
        ▼
 Transports (MC / S2 / S0 / TS) ── may re-enqueue children
        │
        ▼
 zwave_api_transport ── ONE transmission_ongoing
        │
        ▼
 zwapi_session / connection ── sync REQ → ACK → RES → [idle] → CB REQ
        │
        ▼
 UART 115200 8N1 ↔ Z-Wave NCP
```

| Layer | In-flight limit | Notes |
|-------|-----------------|-------|
| `zwave_tx_process` | 1 session chain | Parent/child lock (intentional priority inversion) |
| `zwave_api_transport` | 1 RF TX | Second call → `SL_STATUS_BUSY` |
| Serial session | 1 host frame | Fully synchronous send + wait |
| Soft TX queue | 64 | `ZWAVE_TX_QUEUE_BUFFER_SIZE` |
| Session RX queue | 30 | Drop-on-full after ACK already sent |

Host code uses classic `ZW_SendData` / `SendDataMulti` / `SendNop` — not
`ZW_SendDataEx`.

---

## Findings (ranked)

### P0 — Transport BUSY discarded frames (`zwave_tx_process`)

`zwave_tx_process_send_next_message_step()` treated any non-OK transport status
(including `SL_STATUS_BUSY`) as fatal and called
`zwave_tx_drop_unsent_current_message()`. Under S2/API contention this silently
fails frames and pushes wasteful re-enqueue traffic onto the Serial API.

**Fix in patch:** on `SL_STATUS_BUSY`, leave the frame queued, return to IDLE,
short ~20 ms back-off, retry.

### P0 / P1 — Unconditional Explore (`zwave_api_transport`)

Non-fasttrack singlecast (and NOP intercept) always set
`TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_EXPLORE`. Explore is expensive in
airtime and holds the single Serial API RF session far longer than a routed
retry when a path is already known.

**Fix in patch:** added `zwave_tx_transport_options_t.omit_explore`. When the Tx
route cache has a multi-hop entry for the destination, `zwave_tx_process` sets
`omit_explore` and the API transport skips Explore (AUTO_ROUTE only).

### P1 — zwapi session retries / busy-wait / dead-port recovery

| Issue | Before | After (patch) |
|-------|--------|----------------|
| `MAX_TRANSMISSION_RETRIES` | 20 (INS12350 says 3) | **3** |
| Retransmit back-off | 20 ms busy spin | capped INS12350-style wait with UART sleep |
| ACK wait | busy-spin `select(0)` | `zwapi_serial_wait_for_data(5)` |
| Idle UART ACK timeout | never fired in connection layer | evaluated outside select-ready loop |
| Unexpected hunt byte | forced `TX_TIMEOUT` | ignored until real ACK timeout |
| `RX_TIMEOUT` while waiting ACK | not counted toward reopen | counts toward `MAX_TX_TIMEOUTS` |
| RX frame alloc | always `malloc(255)` | allocate exact frame length |

### P2 — Remaining structural limits (not fully fixed here)

These need larger design work (prefer upstream ZPC repo):

1. **Global response back-off** — successful singlecast with
   `number_of_responses > 0` freezes the entire TX SM for
   `N * (tx_time + 1s)`. Allow TX to *other* NodeIDs while waiting for replies
   from Node A (largest remaining UART-idle win at ~200 nodes).
2. **Attribute resolver** (Unify) — on `SL_STATUS_NOT_READY` skips rather than
   queues; couples poorly with depth-64 TX queue.
3. **Route cache** — only stores multi-hop (`repeaters > 0`); direct neighbors
   never suppress Explore. Extend cache / mark last successful TX including
   0-hop.
4. **NOP Explore** — improved when `omit_explore` is set; still Explore by
   default. Rate-limit / coalesce pending NOPs per NodeID.
5. **RX path** — `zwave_rx_map_node_information()` sync-calls
   `zwapi_get_protocol_info()` during NIF handling (extra Serial RTT mid-RX).
6. **Blocking TTY** — `zwapi_serial` clears `O_NONBLOCK`; `get_buffer` can
   `exit(1)` / hang on short reads.
7. **Fixed 115200** — modest win for NVM bulk; small for short cmds.
8. **No true Serial API pipelining** — host protocol is stop-and-wait; needs
   NCP support for windowing.

---

## Bandwidth mental model

At 115200 8N1 (~11.5 kB/s), a short Serial API frame is ~1–3 ms on the wire.
Round-trip wait + RF dwell + Explore + response back-off dominate. Improving
utilization means **keeping the pipe filled with useful REQs** and **shortening
RF-held sessions**, not raising baud first.

---

## Validation suggestions

1. Unit: existing `zwave_tx` / `zwave_api_transport` / `zwapi_*` tests after patch.
2. Host load kit (this repo): `zwave_load_benchmark_test`, `tools/mqtt_load_test.py`.
3. Hardware: serial log + `scripts/serial_decode_zpc.py`; watch CAN/NAK rates,
   Explore frequency, `[zwave_tx]` back-off reasons, NCP Diagnostics
   `PHYToMACQueueLimitReached`.

---

## Related paths

| Area | Path |
|------|------|
| Serial session | `applications/zpc/components/zwave_api/src/zwapi_session.c` |
| Connection FSM | `.../zwapi_connection.c` |
| POSIX UART | `.../platform/posix/zwapi_serial.c` |
| TX Contiki SM | `.../zwave/zwave_tx/src/zwave_tx_process.cpp` |
| API transport | `.../zwave_api_transport/src/zwave_api_transport.c` |
| Route cache | `.../zwave_tx/src/zwave_tx_route_cache.*` |
| Load kit (Unify) | `components/uic_attribute_resolver/test/zwave_load_benchmark_test.c` |
