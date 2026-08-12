# Architecture {#zpc_architecture}

ZPC follows the Unify fixture-based initialization pattern. Components are brought up
in a strict order defined in `applications/zpc/main.c`.

## High-Level Data Flow

```
  Z-Wave NCP                    ZPC Application                    Unify Ecosystem
 +-----------+    Serial API    +------------------+    MQTT       +-------------+
 |  Z-Wave   | <--------------> |  Z-Wave RX/TX    | <-----------> | MQTT Broker |
 |  Module   |                  |  Command Classes |   DotDot/UCL | GMS, Dev UI |
 +-----------+                  |  Attribute Store |              | UPVL, ...   |
                                |  DotDot Mapper   |              +-------------+
                                +------------------+
```

## Component Layers

| Layer | Components | Responsibility |
|-------|------------|----------------|
| Transport | `zwave_api`, `zwave_rx`, `zwave_tx`, `zwave_transports` | Serial API to NCP |
| Network | `zwave_network_management`, `zwave_s2`, `zwave_smartstart_management` | Network lifecycle |
| Command handling | `zwave_command_classes`, `zwave_command_handler`, `zpc_rust` | Parse/build Z-Wave frames |
| Data model | `attribute_store`, `zpc_attribute_store`, `zpc_attribute_mapper` | Unified attribute tree |
| Resolution | `zpc_attribute_resolver`, `zwave_poll_manager` | Read/write attribute → Z-Wave |
| DotDot bridge | `dotdot_mapper`, `zcl_cluster_servers`, `zpc_dotdot_mqtt`, `ucl_mqtt` | MQTT serialization |
| Infrastructure | `zpc_config`, `zpc_datastore`, `network_monitor`, `zpc_application_monitoring` | Config, persistence, health |

## Startup Sequence

ZPC boots through `uic_fixt_setup_steps_list` in `main.c`. Order matters — later
fixtures depend on earlier ones.

| Step | Fixture | Purpose |
|------|---------|---------|
| 1 | `zpc_config_fixt_setup` | Load CLI and config file (`uic.cfg`) |
| 2 | `zpc_ncp_update_fixt_setup` | Optional NCP firmware flash |
| 3 | `zpc_application_monitoring_init` | ApplicationMonitoring cluster |
| 4 | `zpc_datastore_fixt_setup` | SQLite persistence |
| 5 | `zwave_rx_fixt_setup` | Connect to Z-Wave API, start RX |
| 6 | `zwave_tx_fixt_setup` | Start Z-Wave TX |
| 7 | `zwave_network_management_fixt_setup` | Network state machine, cache NodeID/HomeID |
| 8 | `zwave_s2_fixt_setup` | S2 security |
| 9 | `initialize_request_poller_process` | Rust task executor |
| 10 | `zwave_transports_init` | Z-Wave transport layer |
| 11 | `attribute_store_init` | Core attribute store (C) |
| 12 | `rust_attribute_store_init` | Rust attribute store bindings |
| 13 | `attribute_transitions_init` | Attribute state transitions |
| 14 | `attribute_timeouts_init` | Attribute timeout handling |
| 15 | `zpc_attribute_resolver_init` | Attribute resolution engine |
| 16 | `zwave_poll_manager_init` | Attribute polling |
| 17 | `network_monitor_setup_fixt` | Activate ZPC network UNID |
| 18 | `zpc_dotdot_mqtt_init` | DotDot MQTT topic handlers |
| 19 | `dotdot_mapper_init` | DotDot ↔ Z-Wave cluster mappers |
| 20 | `zcl_cluster_servers_init` | Simulated ZCL clusters |
| 21 | `ucl_mqtt_setup_fixt` | UCL MQTT handlers |
| 22 | `unify_dotdot_attribute_store_init` | DotDot attribute store specialization |
| 23 | `uic_mqtt_dotdot_init` | DotDot MQTT serialization |
| 24 | `zwave_command_class_init_rust_handlers` | Rust command class handlers |
| 25 | `zwave_command_class_init_rust_handlers_legacy` | Legacy Rust handlers |
| 26 | `zwave_command_classes_init` | C command class handlers |
| 27 | `zwave_command_handler_init` | Command dispatch framework |
| 28 | `zwave_smartstart_management_setup_fixt` | SmartStart inclusion |
| 29 | `zpc_stdin_setup_fixt` | Interactive CLI |
| 30 | `zpc_attribute_mapper_init` | UAM rule engine |
| 31 | `zpc_attribute_store_init` | Final ZPC attribute store callbacks |

## Shutdown Sequence

Teardown runs in reverse dependency order via `uic_fixt_shutdown_steps_list`:

1. ZCL Cluster servers
2. Attribute resolver
3. Command handler framework
4. Network management
5. Z-Wave RX
6. Attribute transitions / timeouts
7. Rust mainloop
8. Attribute store
9. Datastore
10. DotDot mapper

## Entry Point

```c
int main(int argc, char **argv)
{
  attribute_mapper_config_init();
  if (zpc_config_init()) {
    return -1;
  }
  return uic_main(uic_fixt_setup_steps_list,
                  uic_fixt_shutdown_steps_list,
                  argc, argv,
                  CMAKE_PROJECT_VERSION);
}
```

## Related Pages

- [Core Components](@ref zpc_components)
- [Directory Structure](@ref zpc_directory_structure)
- [DotDot Mapping](@ref zpc_dotdot_mapping)
