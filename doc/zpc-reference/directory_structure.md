# Directory Structure {#zpc_directory_structure}

ZPC source layout as of Unify SDK tag `ver_1.6.0` under `applications/zpc/`.

## Top-Level Layout

```
applications/zpc/
├── main.c                    # Entry point and fixture lists
├── CMakeLists.txt            # Build target and Debian packaging (uic-zpc)
├── Cargo.toml                # Rust workspace definition
├── Cargo.lock
├── applications/             # Auxiliary tools
│   ├── zpc_database_tool/  # SQLite DB upgrade/recovery
│   └── zwave_api_demo/     # Low-level Serial API demo
├── components/               # Core ZPC libraries (23 subdirectories)
├── debconf/                  # Debian package configuration templates
├── doc/                      # Doxygen docs, UML diagrams, assets
└── scripts/                  # systemd, udev, bash-completion, certification
```

## Components Directory

```
applications/zpc/components/
├── dotdot_mapper/            # DotDot cluster → Z-Wave CC mapping rules
├── network_monitor/          # Network state and UNID management
├── rust_command_class_frame_types/
├── ucl_mqtt/                 # UCL-specific MQTT handlers
├── zcl_cluster_servers/      # Simulated ZCL clusters
├── zpc_application_monitoring/
├── zpc_attribute_mapper/     # UAM rule engine initialization
├── zpc_attribute_resolver/   # Attribute read/write resolution
├── zpc_attribute_store/      # ZPC-specific attribute store layer
├── zpc_config/               # CLI and config file parameters
├── zpc_datastore/            # Persistent SQLite storage
├── zpc_dotdot_mqtt/          # DotDot MQTT topic handlers
├── zpc_ncp_update/           # NCP firmware flashing
├── zpc_rust/                 # Rust CC framework, poll manager
├── zpc_stdin/                # Interactive CLI commands
├── zpc_utils/                # Shared utilities
├── zwave/                    # Z-Wave stack (9 subdirectories)
├── zwave_api/                # Serial API to Z-Wave NCP
├── zwave_command_classes/    # ~50 command class handlers
├── zwave_command_handler/    # Command dispatch framework
├── zwave_rust_proc_macros/
├── zwave_rust_proc_macros_legacy/
└── zwave_smartstart_management/
```

## Z-Wave Stack (`components/zwave/`)

| Subdirectory | Role |
|--------------|------|
| `zwave_controller` | Controller-level operations |
| `zwave_definitions` | Z-Wave protocol definitions |
| `zwave_network_management` | Inclusion, exclusion, node info |
| `zwave_rx` | Incoming frame processing |
| `zwave_security_validation` | Security frame validation |
| `zwave_transports` | Transport layer abstraction |
| `zwave_tx` | Outgoing frame transmission |
| `zwave_tx_groups` | TX group management |
| `zwave_tx_scheme_selector` | TX scheme selection |

## Scripts and Packaging

| Path | Purpose |
|------|---------|
| `scripts/systemd/uic-zpc.service` | systemd unit file |
| `scripts/udev/99-leds.rules` | LED udev rules (Raspberry Pi identify) |
| `scripts/bash-completion/zpc` | Shell tab completion |
| `scripts/certification/` | Python scripts for Z-Wave certification testing |
| `debconf/` | Debian `config`, `postinst`, `templates` for `uic-zpc` package |

## Rust Workspace Members

Defined in `applications/zpc/Cargo.toml`:

- `components/zwave_rust_proc_macros`
- `components/zwave_rust_proc_macros_legacy`
- `components/rust_command_class_frame_types`
- `components/zpc_rust`

## Related Pages

- [Core Components](@ref zpc_components)
- [Build and Deployment](@ref zpc_build_deploy)
