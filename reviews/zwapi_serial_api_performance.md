# zwapi Serial API Performance Review

Review of `applications/zpc/components/zwave_api` (zwapi) focused on host↔NCP
Serial API protocol efficiency and UART utilization.

**Baseline:** Unify SDK **`ver_1.6.0`**. Companion review:
[zwave_tx_rx_performance.md](zwave_tx_rx_performance.md).

**Fixes:** `tools/zpc_serial_api_perf/0001-zwave-zwapi-serial-api-performance.patch`
(see `tools/zpc_serial_api_perf/README.md`).

---

## Executive verdict

**115200 baud is not the primary limit.** zwapi implements classic INS12350
stop-and-wait: one outstanding host REQ (SOF frame → ACK/NAK/CAN), optional RES
wait, then Contiki resumes. Short frames are ~1–3 ms on the wire; round-trip
waits, busy-spins, and retry storms dominate.

---

## How the link works

Wire format (`zwapi_connection_tx`):

`SOF (0x01) | Len | Type | Cmd | DATA… | XOR checksum`

| Symbol | Role |
|--------|------|
| ACK `0x06` | Frame accepted |
| NAK `0x15` | Bad checksum / reject → retransmit |
| CAN `0x18` | Collision / host frame dropped |

Session APIs:

- `zwapi_session_send_frame` — TX + wait ACK
- `zwapi_session_send_frame_with_response` — ACK + matching RES
- `zwapi_session_send_frame_no_ack` — soft-reset style

Unsolicited NCP REQs are enqueued (`MAX_RX_QUEUE_LENGTH` = 30) and drained via
`zwapi_poll()` → `zwave_api_protocol_rx_dispatch()`.

**Only one host→NCP Serial API frame may be outstanding.** There is no TX
queue or pipelining in zwapi.

---

## Findings (ranked)

### P1 — Retries / busy-wait / dead-port recovery (`zwapi_session.c`)

| Issue | Before | After (patch) |
|-------|--------|----------------|
| `MAX_TRANSMISSION_RETRIES` | 20 (INS12350 says 3) | **3** |
| Retransmit back-off | 20 ms busy spin | capped INS12350-style wait + UART sleep |
| ACK wait | busy-spin `select(0)` | `zwapi_serial_wait_for_data(5)` |
| `RX_TIMEOUT` while waiting ACK | not counted toward reopen | counts toward `MAX_TX_TIMEOUTS` |
| RX frame alloc | always `malloc(255)` | allocate exact frame length |

### P1 — ACK timeout when UART idle (`zwapi_connection.c`)

`timeOutACK` was only evaluated inside the “bytes ready” loop. A silent NCP
never produced connection-layer `TX_TIMEOUT`; the session outer timer fired as
`RX_TIMEOUT` instead, which previously did not count toward port reopen.

**Fix:** evaluate ACK timeout outside the select-ready loop.

Unexpected non-ACK/NAK/CAN bytes while hunting used to force `TX_TIMEOUT`;
they are now ignored until the real ACK timeout.

### P2 — POSIX serial (`zwapi_serial.c`)

- Baud fixed at **B115200** (modest win to raise for NVM bulk only).
- Open clears `O_NONBLOCK`; `get_buffer` can block forever / `exit(1)` on error.
- `tcdrain` after every TX and every ACK/NAK adds frame-time latency.
- Patch adds `zwapi_serial_wait_for_data(timeout_ms)` for event-driven waits.

### P2 — Other zwapi limits

- `FUNC_ID_SERIAL_API_SET_TIMEOUTS` defined but unused.
- Drop-on-full RX queue (depth 30) after ACK already sent to NCP.
- No ReceiveStatus / ApplicationUpdate rate limiting in dispatch (NIF floods).
- True pipelining needs NCP support — not a host-only change.

---

## Bandwidth mental model

Effective sync-RPC rate ≈ `1 / (T_tx + T_ack + T_res + software_wait)`.
Improving utilization here means **shorter waits**, **fewer useless retries**,
and **correct timeouts** — not raising baud first.

---

## Key paths

| Area | Path |
|------|------|
| Session | `applications/zpc/components/zwave_api/src/zwapi_session.c` |
| Connection FSM | `.../zwapi_connection.c` |
| POSIX UART | `.../platform/posix/zwapi_serial.c` |
| RX dispatch | `.../zwapi_protocol_rx_dispatch.c` |
| Transport cmds | `.../zwapi_protocol_transport.c` |
