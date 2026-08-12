# Build and Deployment {#zpc_build_deploy}

Instructions for building and deploying ZPC from the Unify SDK (tag `ver_1.6.0` or
earlier releases that include `applications/zpc/`).

## Prerequisites

- CMake 3.19+ (ZPC) / 3.21+ (root Unify SDK)
- C/C++ toolchain with Rust support (`enable_language(Rust)`)
- Doxygen (for generating this reference documentation)
- Z-Wave NCP hardware connected via serial

## CMake Build

### Enable ZPC

```cmake
option(BUILD_ZPC "Package the ZPC" ON)
```

In `applications/CMakeLists.txt`:

```cmake
if(BUILD_ZPC MATCHES ON)
  add_subdirectory(zpc)
endif()
```

### Build Commands

```bash
mkdir build && cd build
cmake -DBUILD_ZPC=ON ..
make zpc
```

### Linked Libraries

The `zpc` executable links against:

| Library | Purpose |
|---------|---------|
| `dotdot_mapper` | DotDot cluster mapping |
| `network_monitor` | Network state management |
| `ucl_mqtt` | UCL MQTT handlers |
| `zcl_cluster_servers` | Simulated ZCL clusters |
| `zpc_attribute_mapper` | UAM rule engine |
| `zpc_attribute_resolver` | Attribute resolution |
| `zpc_attribute_store` | ZPC attribute store |
| `zpc_config` | Configuration |
| `zpc_datastore` | SQLite persistence |
| `zpc_dotdot_mqtt` | DotDot MQTT |
| `zpc_ncp_update` | NCP firmware update |
| `zpc_rust` | Rust components |
| `zwave_controller` | Z-Wave controller |
| `zwave_command_classes` | Command class handlers |
| `zwave_command_handler` | Command dispatch |
| `zwave_network_management` | Network management |
| `zwave_rx` / `zwave_tx` | Frame I/O |
| `zwave_s2` | S2 security |
| `zwave_smartstart_management` | SmartStart |
| `zwave_api` | Serial API |

## Rust Workspace

`applications/zpc/Cargo.toml` defines the workspace:

```toml
[workspace]
members = [
    "components/zwave_rust_proc_macros",
    "components/zwave_rust_proc_macros_legacy",
    "components/rust_command_class_frame_types",
    "components/zpc_rust",
]
```

Release profile enables `debug-assertions` and LTO for Debian packaging.

## Debian Package (`uic-zpc`)

CMake installs the following via `add_component_to_uic()`:

| Artifact | Install Path |
|----------|-------------|
| `zpc` binary | `/usr/bin/zpc` |
| systemd service | `/lib/systemd/system/uic-zpc.service` |
| UAM rules | `/usr/share/uic/rules/*.uam` |
| bash completion | `/share/bash-completion/completions/zpc` |
| udev rules | `/etc/udev/rules.d/99-leds.rules` |
| data directory | `/var/lib/zpc` |

### systemd Service

```ini
[Unit]
Description=Unify Z-Wave protocol controller
After=network.target mosquitto.service

[Service]
WorkingDirectory=/var/lib/uic/zpc
ExecStart=/usr/bin/zpc
User=uic
Group=uic
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

## Generate This Documentation

From a configured CMake build directory:

```bash
cmake ..
make doxygen_zpc_reference
```

Output HTML is written to:

```
build/doxygen_zpc_reference/html/index.html
```

Serve locally:

```bash
cd build/doxygen_zpc_reference/html
python3 -m http.server 8080
```

Then open http://localhost:8080 in a browser.

## Generate ZPC API Documentation

Requires `applications/zpc/` source (checkout tag `ver_1.6.0`):

```bash
git checkout ver_1.6.0 -- applications/zpc/
cmake -DBUILD_ZPC=ON ..
make doxygen_zpc
```

Output: `build/doxygen_zpc/html/index.html`

## Related Pages

- [Repository Status](@ref zpc_repository_status)
- [Configuration](@ref zpc_configuration)
- [Tools and Utilities](@ref zpc_tools)
