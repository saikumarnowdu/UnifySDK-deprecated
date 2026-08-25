# ZPC tools and patch kits

Helper patches and reference material for Z-Wave Protocol Controller (ZPC) work in this repository.

## ZPC repository map

Silicon Labs has shipped ZPC under several names and repositories over time. Use this map to find the right tree for your version.

| Repository | Tag / branch | Era | Notes |
|------------|--------------|-----|-------|
| [SiliconLabs/UnifySDK](https://github.com/SiliconLabs/UnifySDK) | `release/1.6.x`, `ver_1.6.0` | 2021–2024 | ZPC lived under `applications/zpc/` inside Unify Host SDK. This fork (`UnifySDK-deprecated`) carries the same layout on `release/1.6.x`. |
| [SiliconLabs/UnifySDK](https://github.com/SiliconLabs/UnifySDK) | `ver_1.1.0` … `ver_1.1.1` | 2022 | Original home of the **1.1.1 Binary Switch / no-supervision** fix. See [compare `ver_1.1.0...ver_1.1.1`](https://github.com/SiliconLabs/UnifySDK/compare/ver_1.1.0...ver_1.1.1). |
| [z-wave-protocol-controller-legacy](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller-legacy) | `ver_1.6.0` … `ver_1.7.x` | 2024–2025 | ZPC split out of UnifySDK; still legacy API and `applications/zpc/` layout. Release notes: [`applications/zpc/release_notes.md`](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller-legacy/blob/main/applications/zpc/release_notes.md). |
| [z-wave-protocol-controller](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller) | `zpc-v2.0.0` | 2026+ | **New rewrite** (beta). Different MQTT API and `components/` layout at repo root. **Not** API-compatible with Unify ZPC 1.6.x. Legacy releases: [z-wave-protocol-controller-legacy/releases](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller-legacy/releases). |

### Which tree should I use?

| Goal | Use |
|------|-----|
| Unify / UCL MQTT, `release/1.6.x` gateways | This repo (`applications/zpc/`) or `z-wave-protocol-controller-legacy` |
| Investigate the 2022 no-supervision OnOff fix | UnifySDK `ver_1.1.0` → `ver_1.1.1` diff (see below) |
| New Silicon Labs ZPC beta (2026) | `z-wave-protocol-controller` tag `zpc-v2.0.0` + patch in `tools/zpc_v2_supervision_no_get/` |

### Legacy 1.1.1 fix (no-supervision Binary Switch)

Documented in [`applications/zpc/release_notes.md`](../applications/zpc/release_notes.md) under **Fixed (1.1.1)**:

> ZPC stops controlling OnOff if Binary Switch Set without supervision failed.

**Code diff:** [UnifySDK compare `ver_1.1.0...ver_1.1.1`](https://github.com/SiliconLabs/UnifySDK/compare/ver_1.1.0...ver_1.1.1) — mainly `zwave_command_class_binary_switch.c` (align desired when REPORT has no target value; only schedule duration timeout when desired ≠ reported).

**Behavior docs:** [ZPC User Guide — attribute resolver / supervision](https://siliconlabssoftware.github.io/z-wave-protocol-controller/applications/zpc/readme_user.html).

---

## Patch kits in this repo

| Directory | Target | Description |
|-----------|--------|-------------|
| [`zpc_v2_supervision_no_get/`](zpc_v2_supervision_no_get/README.md) | `z-wave-protocol-controller` `zpc-v2.0.0` | Resolver-only patch: skip redundant GET after no-supervision SET when `reported` already matches |

### Unify ZPC 1.6.x (in-tree fixes)

Branch `cursor/binary-switch-no-get-03e8` (PR against `release/1.6.x`) includes:

- Generic resolver fix (`components/uic_attribute_resolver/`)
- Binary Switch verify-then-decide (`ZWAVE_RECOMMENDED_RESPONSE_TIME_MS` = 1600ms) in `applications/zpc/components/zwave_command_classes/`
- `zpc_attribute_resolver_send.c`: wait for one application response on no-supervision SET

---

## Related docs in this repo

- [`applications/zpc/release_notes.md`](../applications/zpc/release_notes.md) — ZPC changelog (includes 1.1.1 entry)
- [`applications/zpc/doc/mqtt_to_serial_flow.md`](../applications/zpc/doc/mqtt_to_serial_flow.md) — outbound MQTT → serial
- [`applications/zpc/doc/serial_to_mqtt_flow.md`](../applications/zpc/doc/serial_to_mqtt_flow.md) — inbound serial → MQTT
- [`reviews/`](../reviews/README.md) — zwapi / zwave Serial API performance notes
