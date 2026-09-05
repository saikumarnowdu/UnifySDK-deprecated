# Standalone resolver → TX queue dashboard (no NCP)

Build and run from the Unify tree:

```bash
make -C applications/zpc/tools/zwave_tx_observability
./applications/zpc/tools/zwave_tx_observability/zwave_tx_obs_sim
```

Open `http://127.0.0.1:8765/`.

This process uses the real `queue_element_send_compare` and `zwave_tx_route_cache_link_score()` from ZPC. Radio, MQTT, and serial are simulated so you can watch pacing and send order without deploying.

## Application tracing (real ZPC)

Production ZPC now emits one **trace ID** per resolver request. Spans:

| Span | Where |
|---|---|
| `begin` / `resolver.SET` or `resolver.GET` | `attribute_resolver_send()` |
| `resolver.defer` | TX queue full (`NOT_READY`) |
| `tx.enqueue` | `zwave_tx_send_data()` |
| `tx.on_air` | `zwave_tx_process` hands frame to transports |
| `tx.complete` | radio callback |
| `resolver.complete` / `end` | resolver TX callback |

Log tag: `zpc_trace`. Example:

```bash
grep zpc_trace zpc.log | grep '"trace":"00000000000000ab"'
```

| Button | What it does |
|---|---|
| Burst 232 SETs | Resolver scan: enqueue until 63, defer the rest (`NOT_READY`) |
| Seed last-TX metrics | Fill hops/speed/RSSI/beam as if nodes already answered once |
| Second burst | Same SETs again so send order uses the radio cache |
| Pause / resume | Freeze the sim clock |
