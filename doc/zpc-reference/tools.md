# Tools and Utilities {#zpc_tools}

Auxiliary applications and scripts shipped with ZPC under `applications/zpc/`.

## zpc_database_tool

**Path:** `applications/zpc/applications/zpc_database_tool/`

SQLite database management utilities for the ZPC datastore.

| Binary | Purpose |
|--------|---------|
| `zpc_database_tool` | General database operations |
| `zpc_database_upgrade_tool` | Upgrade database schema between versions |
| `zpc_database_recover_tool` | Recover corrupted databases |

Key classes:

- `zpc_database_helper` — low-level database access
- `zpc_database_updater` — schema migration logic

Includes unit tests with sample database assets (`test/assets/v2.db`).

## zwave_api_demo

**Path:** `applications/zpc/applications/zwave_api_demo/`

Low-level demonstration of the Z-Wave Serial API, independent of the full ZPC stack.

```bash
zwave_api_demo -s /dev/ttyUSB0
```

| Flag | Description |
|------|-------------|
| `-s` | Serial device path for the Z-Wave module |

Source files:

- `zwave_api_demo.c` — main entry and argument parsing
- `zwave_api_demo_callbacks.c` — API callback handlers
- `zwave_api_demo_commands.c` — interactive commands

## Certification Scripts

**Path:** `applications/zpc/scripts/certification/`

Python scripts for Z-Wave certification testing (CTT integration).

```
scripts/certification/
├── command_classes/
│   ├── clusters/        # Level, OnOff cluster tests
│   ├── config/          # Unify config helpers
│   ├── mqtt/            # MQTT manager for tests
│   └── sound_switch.py
├── config.ini
├── requirements.txt
├── readme_dev.md
└── readme_user.md
```

Install dependencies:

```bash
pip install -r scripts/certification/requirements.txt
```

## Shell Scripts

| Script | Purpose |
|--------|---------|
| `scripts/zpc/node_identify_rpi4_led.sh` | Blink Raspberry Pi 4 LEDs for node identify |
| `scripts/bash-completion/zpc` | Bash tab completion for the `zpc` CLI |

## Command Class Generator

**Path:** `applications/zpc/components/zwave_command_classes/scripts/`

| Script | Purpose |
|--------|---------|
| `generator.py` | Generate CC handlers from XML model |
| `notification.py` | Notification command class helpers |

See [Command Classes](@ref zpc_command_classes) for usage.

## Rust Code Generation

**Path:** `applications/zpc/components/zpc_rust/`

| File | Purpose |
|------|---------|
| `zwave_rust_cc_framework_gen.py` | Generate Rust command class framework |
| `zwave_poll_config.yaml` | Poll manager configuration |
| `build.rs` | Cargo build script |

## Related Pages

- [Command Classes](@ref zpc_command_classes)
- [Build and Deployment](@ref zpc_build_deploy)
- [Directory Structure](@ref zpc_directory_structure)
