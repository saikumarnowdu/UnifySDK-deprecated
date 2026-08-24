# zwave TX/RX Performance Review

Review of `applications/zpc/components/zwave` (TX, RX, API transport, route
cache) focused on keeping the Serial API busy with useful work under high node
counts (~200).

**Baseline:** Unify SDK **`ver_1.6.0`**. Companion review:
[zwapi_serial_api_performance.md](zwapi_serial_api_performance.md).

**Fixes:** `tools/zpc_serial_api_perf/0001-zwave-zwapi-serial-api-performance.patch`
(see `tools/zpc_serial_api_perf/README.md`).

---

## Executive verdict

The Contiki TX path is **strictly single-session**: one `current_tx_session_id`,
one `zwave_api_transport` RF in-flight slot, feeding one Serial API SendData.
Under load the UART is often **idle waiting for RF callbacks and response
back-offs**, not saturated. Worst waste: dropping on transient `BUSY`, always-on
Explore, and global reply waits that block all other NodeIDs.

---

## Data path

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
 zwapi (see companion review) ── sync SendData → CB REQ
```

| Layer | In-flight limit | Notes |
|-------|-----------------|-------|
| `zwave_tx_process` | 1 session chain | Parent/child lock (priority inversion by design) |
| `zwave_api_transport` | 1 RF TX | Second call → `SL_STATUS_BUSY` |
| Soft TX queue | 64 | `ZWAVE_TX_QUEUE_BUFFER_SIZE` |
| Expected-RX tracking | 10 NodeIDs | `ZWAVE_TX_INCOMING_FRAMES_BUFFER_SIZE` |
| Route cache | 50 NodeIDs | multi-hop repeaters only |

Host uses classic `ZW_SendData` / `SendDataMulti` / `SendNop` — not
`ZW_SendDataEx`.

---

## Findings (ranked)

### P0 — Transport BUSY discarded frames (`zwave_tx_process.cpp`)

`zwave_tx_process_send_next_message_step()` treated any non-OK transport status
(including `SL_STATUS_BUSY`) as fatal and called
`zwave_tx_drop_unsent_current_message()`. Under S2/API contention this silently
fails frames and forces upper-layer retries (more Serial API load).

**Fix in patch:** on `BUSY`, leave the frame queued, return to IDLE, ~20 ms
back-off, retry. Permanent failures still discard.

### P0 / P1 — Unconditional Explore (`zwave_api_transport.c`)

Non-fasttrack singlecast and NOP intercept always set
`TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_EXPLORE`. Explore holds RF and
the single Serial API session far longer than a routed retry when a path is
known.

**Fix in patch:** `zwave_tx_transport_options_t.omit_explore`. When the route
cache has a multi-hop entry, `zwave_tx_process` sets the flag and the API
transport skips Explore (AUTO_ROUTE only).

### P2 — Global response back-off (not fixed)

Successful singlecast with `number_of_responses > 0` freezes the entire TX SM
for `N * (tx_time + 1s)`. **Largest remaining scale win:** allow SendData to
other NodeIDs while waiting for replies from Node A.

### P2 — Other zwave limits

1. **Route cache** only stores `repeaters > 0`; direct neighbors never suppress
   Explore. Extend with last-success including 0-hop.
2. **NOP floods** still Explore by default; coalesce/rate-limit per NodeID.
3. **RX** — `zwave_rx_map_node_information()` sync-calls
   `zwapi_get_protocol_info()` during NIF handling (extra Serial RTT mid-RX).
4. **Queue depth 64** shallow for interview/poll bursts; no shed-by-destination.
5. **Attribute resolver** (Unify) skips on `NOT_READY` instead of queueing.
6. Parent/child chain lock: intentional, but blocks higher-QoS work to other
   nodes during long S2/TS/MC chains.

---

## Ranked follow-ups

1. Per-destination reply wait (not global TX lock)
2. Mark 0-hop successful TX in route cache → more Explore suppression
3. Defer `get_protocol_info` out of RX/NIF path
4. Smarter TX queue shed policy + optional depth increase
5. Multicast groups for identical multi-node payloads
6. Consider `SendDataEx` if NCP supports richer routing/speed options

---

## Validation

1. Unit: `zwave_tx` / `zwave_api_transport` tests after patch.
2. Host load kit: `zwave_load_benchmark_test`, `tools/mqtt_load_test.py`.
3. Hardware: serial log + `scripts/serial_decode_zpc.py`; watch Explore rate,
   `[zwave_tx]` back-off reasons, CAN/NAK, NCP
   `PHYToMACQueueLimitReached`.

---

## Key paths

| Area | Path |
|------|------|
| TX Contiki SM | `applications/zpc/components/zwave/zwave_tx/src/zwave_tx_process.cpp` |
| TX queue | `.../zwave_tx_queue.*` |
| Route cache | `.../zwave_tx_route_cache.*` |
| API transport | `.../zwave_api_transport/src/zwave_api_transport.c` |
| RX | `.../zwave_rx/` |
| Options | `.../zwave_definitions/include/zwave_tx_definitions.h` |
| Load kit (Unify) | `components/uic_attribute_resolver/test/zwave_load_benchmark_test.c` |
