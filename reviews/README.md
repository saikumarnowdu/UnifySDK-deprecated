# Reviews

Performance and architecture reviews for ZPC **zwave** / **zwapi** layers.

| Review | Scope |
|--------|--------|
| [zwapi_serial_api_performance.md](zwapi_serial_api_performance.md) | Serial API host stack (`zwave_api` / zwapi session, connection, UART) |
| [zwave_tx_rx_performance.md](zwave_tx_rx_performance.md) | Z-Wave TX/RX Contiki path, API transport, Explore, back-off |

**Baseline:** Unify SDK `ver_1.6.0` (`applications/zpc`; removed in `ver_1.7.0`).

**Patch kit:** `tools/zpc_serial_api_perf/` — applyable fixes from both reviews.
