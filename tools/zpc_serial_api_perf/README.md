# ZPC Serial API performance patch

Patch produced from the performance reviews of **zwave** / **zwapi** Serial API
utilization:

- [reviews/zwapi_serial_api_performance.md](../../reviews/zwapi_serial_api_performance.md)
- [reviews/zwave_tx_rx_performance.md](../../reviews/zwave_tx_rx_performance.md)
- [reviews/README.md](../../reviews/README.md)

## Apply

Against a tree that still contains `applications/zpc` (e.g. Unify
`ver_1.6.0`, or a checkout of the standalone Z-Wave Protocol Controller with
matching paths):

```bash
# From the repository root that contains applications/zpc/
patch -p1 < tools/zpc_serial_api_perf/0001-zwave-zwapi-serial-api-performance.patch
```

Dry-run:

```bash
patch -p1 --dry-run < tools/zpc_serial_api_perf/0001-zwave-zwapi-serial-api-performance.patch
```

## What it changes

| File | Change |
|------|--------|
| `zwave_tx_process.cpp` | Retry on transport `BUSY` instead of drop; set `omit_explore` from route cache |
| `zwave_api_transport.c` | Honor `omit_explore` for SendData / NOP |
| `zwave_tx_definitions.h` | Add `transport.omit_explore` |
| `zwapi_session.c` | INS12350 retries (3), event-driven wait, sized RX alloc, timeout counting |
| `zwapi_connection.c` | Idle ACK timeout; ignore unexpected hunt bytes |
| `zwapi_serial.h` / `zwapi_serial.c` | Add `zwapi_serial_wait_for_data()` |

## Not included (larger follow-ups)

Per-NodeID TX while waiting for responses, route-cache 0-hop tracking,
non-blocking UART assembly, baud negotiation, `SendDataEx`.
