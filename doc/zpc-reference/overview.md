# Overview {#zpc_overview}

ZPC (**Z-Wave Protocol Controller**) is the Unify SDK application that acts as a
**Z-Wave gateway/controller** on Linux. It bridges Z-Wave end devices to the Unify
ecosystem over MQTT using the Unified Controller Language (UCL) and DotDot data model.

## Purpose

ZPC performs four primary roles:

1. **Hardware interface** — communicates with a Z-Wave NCP (Network Co-Processor) over
   the Z-Wave Serial API.
2. **Network management** — handles inclusion, exclusion, S2 security, and SmartStart.
3. **Protocol translation** — maps Z-Wave Command Classes to DotDot/UCL attributes via
   the Attribute Mapper (`.uam` rules).
4. **MQTT integration** — publishes device state and accepts commands for other Unify
   services (GMS, Dev UI, UPVL, and similar).

ZPC is the Z-Wave counterpart to ZigPC for Zigbee within the Unify SDK.

## Source Location

| Path | Description |
|------|-------------|
| `applications/zpc/` | ZPC application root (present through tag `ver_1.6.0`) |
| `applications/zpc/main.c` | Application entry point and fixture initialization order |
| `applications/zpc/components/` | Core ZPC libraries |
| `applications/zpc/applications/` | Auxiliary tools and demos |

> **Note:** The folder is `applications/zpc` (plural), not `application/zpc`.

## Technology Stack

| Layer | Technology |
|-------|------------|
| Core runtime | C (Contiki-based event loop via `uic_main`) |
| Command classes | C, C++, and Rust |
| Attribute mapping | UAM (Unify Attribute Mapper) rule files |
| Persistence | SQLite datastore |
| Messaging | MQTT (DotDot + UCL topics) |
| Build system | CMake + Cargo (Rust workspace) |
| Packaging | Debian (`uic-zpc` package) |

## Scale (ver_1.6.0)

Approximately **1,142 files** across the ZPC tree:

| Extension | Count (approx.) |
|-----------|-----------------|
| `.h` | 329 |
| `.c` | 312 |
| `.cpp` | 72 |
| `.uam` | 54 |
| `.rs` | 35 |
| `.py` | 33 |

## Related Documentation

- [Repository Status](@ref zpc_repository_status)
- [Architecture](@ref zpc_architecture)
- [Build and Deployment](@ref zpc_build_deploy)
