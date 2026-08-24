# Core Components {#zpc_components}

Reference for the major ZPC libraries under `applications/zpc/components/`.

## Component Summary

| Component | Role |
|-----------|------|
| `zwave_api` | Serial API interface to the Z-Wave NCP module |
| `zwave/` | RX, TX, controller, network management, security, transports |
| `zwave_command_classes` | Command class handlers (generated + hand-written) |
| `zwave_command_handler` | Command dispatch framework |
| `zwave_smartstart_management` | SmartStart inclusion workflow |
| `zpc_attribute_store` | ZPC-specific attribute store layer |
| `zpc_attribute_mapper` | Initializes the UAM rule engine for ZPC |
| `zpc_attribute_resolver` | Resolves attribute reads/writes to Z-Wave frames |
| `dotdot_mapper` | Maps DotDot clusters to Z-Wave Command Classes via `.uam` rules |
| `zcl_cluster_servers` | Simulated ZCL clusters served on behalf of Z-Wave nodes |
| `ucl_mqtt` | UCL-specific MQTT handlers (non-DotDot) |
| `zpc_dotdot_mqtt` | DotDot MQTT topic serialization |
| `network_monitor` | Network state, UNID assignment, resolver activation |
| `zpc_datastore` | Persistent SQLite storage |
| `zpc_config` | CLI arguments and `uic.cfg` configuration |
| `zpc_ncp_update` | NCP firmware update on startup |
| `zpc_rust` | Rust command class framework, poll manager, attribute store bindings |
| `zpc_application_monitoring` | ApplicationMonitoring cluster for health and MQTT logging |
| `zpc_stdin` | Interactive CLI commands |
| `zpc_utils` | Shared helper functions |

## Z-Wave API

The Z-Wave API (`zwave_api.h`) provides access to the serial API of an attached
Z-Wave module. It is the lowest-level hardware interface used by ZPC.

Key headers:

- `zwapi_init.h` — initialization
- `zwapi_protocol_basis.h` — basic protocol operations
- `zwapi_protocol_controller.h` — controller operations
- `zwapi_protocol_transport.h` — transport layer

## Attribute Store

The attribute store is the central data model. ZPC extends the shared Unify
attribute store with:

- `zpc_attribute_store` — ZPC UNID helpers, network/node/endpoint lookups
- `rust_attribute_store` — Rust bindings (initialized after C attribute store)
- `unify_dotdot_attribute_store` — DotDot specialization

Key functions in `zpc_attribute_store.h`:

- `is_zpc_unid()` — check if a UNID belongs to the ZPC
- `get_zpc_network_node()` — HomeID node in the attribute tree
- `get_zpc_node_id_node()` — controller NodeID node
- `get_zpc_endpoint_id_node()` — endpoint node lookup

## Attribute Resolver

`zpc_attribute_resolver` bridges attribute store changes to Z-Wave TX. It depends on:

- `unify` (shared library)
- `zpc_attribute_store`
- `zwave_tx_groups`
- `zwave_command_classes`

## DotDot Mapper

`dotdot_mapper` contains:

- C/C++ cluster mappers (`basic_cluster_mapper`, `on_off_cluster_basic_mapper`, etc.)
- `dotdot_mapper_binding_cluster_helper` — binding cluster support
- `rules/` — 54 `.uam` attribute mapper rule files (installed to `/usr/share/uic/rules/`)

## Network Monitor

Activates after network management, attribute store, and attribute resolver are ready.
It:

- Sets the ZPC UNID (`zwave_unid`)
- Verifies attribute store consistency
- Pauses resolution for other networks
- Activates the current network for the resolver

## Application Monitoring

Publishes health information over MQTT using the ApplicationMonitoring cluster:

- Application name, version, uptime
- MQTT client ID (e.g. `zpc-<pid>`)
- Connection status and supported MQTT topics

## Related Pages

- [Command Classes](@ref zpc_command_classes)
- [DotDot Mapping](@ref zpc_dotdot_mapping)
- [Configuration](@ref zpc_configuration)
