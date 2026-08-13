# ZPC Fixes Audit for the 1.6.x Product Line

This document records the status of ZPC fixes on `release/1.6.x` (based on
`ver_1.6.0`) and identifies upstream fixes that are **not yet applied** but
should be considered before porting other features from 1.7.0+.

Related guides:

- [Maintaining the Unify SDK 1.6.x Product Line](maintenance_1.6.x.md)
- [ZPC Release Notes](../applications/zpc/release_notes.md)

## Scope

- **In scope:** In-repo ZPC at `applications/zpc/` on `release/1.6.x`
- **Out of scope for direct cherry-pick:** External
  [z-wave-protocol-controller](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller)
  repo (used from Unify 1.7.0 onward). Fixes there must be ported manually into
  the 1.6.x ZPC tree.

Audit date: August 2026. Upstream references: `SiliconLabs/UnifySDK` on GitHub.

---

## Fixes already present on `release/1.6.x`

These are listed in `applications/zpc/release_notes.md` for 1.6.0 and were
verified in the source tree.

| Fix | Ticket / PR | Status on `release/1.6.x` | Key paths |
| --- | ----------- | ------------------------- | --------- |
| ZPC polling mechanism | UIC-2964 / [PR #45](https://github.com/SiliconLabs/UnifySDK/pull/45) | **Present** (via `Release ver_1.6.0`) | `applications/zpc/components/zpc_rust/src/zwave_poll_manager/zwave_poll_register.rs` |
| Send 0xFF by default for endpoint_find | UIC-3160 / [PR #37](https://github.com/SiliconLabs/UnifySDK/pull/37) | **Present** | `applications/zpc/components/zwave_command_classes/src/zwave_command_class_multi_channel.c` |
| MAX_PING_TIME_INTERVAL = 24 hours | [PR #40](https://github.com/SiliconLabs/UnifySDK/pull/40) | **Present** (commit `8c3c67c05`) | `applications/zpc/components/network_monitor/src/failing_node_monitor.h` |
| Prioritize command classes for interview | [PR #39](https://github.com/SiliconLabs/UnifySDK/pull/39) | **Present** (commit `c5cf19cf7`) | `applications/zpc/components/zwave_command_classes/src/zwave_command_class_version.c` |
| Infinite loop on invalid next configuration parameter | [PR #26](https://github.com/SiliconLabs/UnifySDK/pull/26) | **Present** (commit `1daacdf61`) | `applications/zpc/components/zwave_command_classes/src/zwave_command_class_configuration_control.c` |

**Note:** PRs #45 and #37 still show as **OPEN** on upstream GitHub, but their
changes were included in the `Release ver_1.6.0` commit. Do not re-apply those
PRs blindly.

### Polling fix detail (GH-45 / UIC-2964)

The poller now uses the correct Dotdot attribute ID and value type for network
status:

- `DOTDOT_ATTRIBUTE_ID_STATE_NETWORK_STATUS` = `0xfd020001`
- `ZCL_NODE_STATE_NETWORK_STATUS_ONLINE_FUNCTIONAL` = `0` (as `u32`)

Previously the poller used wrong constants (`0x000D` / `u8` = `1`), so polling
did not start when nodes came online.

### Endpoint find fix detail (GH-36/37 / UIC-3160)

`endpoint_find` now defaults to `generic_device_class = 0xFF` and
`specific_device_class = 0xFF`. Legacy devices can opt in via
`ATTRIBUTE_COMMAND_CLASS_MULTI_CHANNEL_FLAG_SEND_TARGETED_DEVICE_CLASS`.

---

## Known issues still open on `release/1.6.x`

### Critical — recommended to port first

| Issue | Description | Upstream fix | Port difficulty |
| ----- | ----------- | ------------ | --------------- |
| **UIC-3335** | Z-Wave TX queue can lock itself; one faulty device can block all Z-Wave traffic | [PR #47](https://github.com/SiliconLabs/UnifySDK/pull/47) — **OPEN** | **Easy** (~10 lines in `zwave_tx.cpp` + tests) |

**Proposed fix (PR #47):** When the TX queue is full, abort transmission of the
first queued frame instead of waiting indefinitely:

```
applications/zpc/components/zwave/zwave_tx/src/zwave_tx.cpp
applications/zpc/components/zwave/zwave_tx/test/zwave_tx_test.c
```

**Recommendation:** Port PR #47 as the first ZPC patch on `release/1.6.x`
(for example `ver_1.6.1`).

### Security — evaluate for 1.6.x line

| CVE | Affected | Fixed in (external ZPC) | Notes |
| --- | -------- | ----------------------- | ----- |
| [CVE-2025-10933](https://nvd.nist.gov/vuln/detail/CVE-2025-10933) | ZPC &lt; 1.7.1 | External repo `ver_1.7.1+` | Integer underflow → out-of-bounds read. **1.6.x is affected.** Locate and port the specific fix from the external z-wave-protocol-controller release. |

### Long-standing known issues (no upstream fix found)

Documented since 1.4.0 in `applications/zpc/release_notes.md`:

| Issue | Summary | Workaround |
| ----- | ------- | ---------- |
| UIC-2274 | Serial-port dialog during install does not update `/etc/uic/uic.cfg` | Manual config edit |
| UIC-2219 | Bindings do not validate full UNID | Do not bind across networks |
| UIC-2283 | Color Switch durations ignored | Set colors instantaneously |
| UIC-1779 | Multi Channel Endpoint Find follow-up reports &gt; 0 not handled | Wait for node recovery |
| UIC-1652 | Invalid YAML silently uses defaults | Validate YAML manually |
| UIC-1088 | Wrong controller NIF after migration | Usually not a problem in practice |

---

## Open upstream PRs — port candidates (after UIC-3335)

These PRs are **OPEN** on `SiliconLabs/UnifySDK` and are **not** in `ver_1.6.0`.
Evaluate each against product requirements before porting.

| PR | Title | Ticket | Scope | Difficulty |
| -- | ----- | ------ | ----- | ---------- |
| [#47](https://github.com/SiliconLabs/UnifySDK/pull/47) | Add fail safe to tx queue | UIC-3335 | ZPC TX only | Easy |
| [#30](https://github.com/SiliconLabs/UnifySDK/pull/30) | Attribute store flag to control supervision | UIC-3181 | ZPC attribute resolver + supervision CC | Medium |
| [#32](https://github.com/SiliconLabs/UnifySDK/pull/32) | Thermostat Mode CC correctly exposed to MQTT | UIC-3069 | ZPC + ZAP/Dotdot regen | Hard (large generated diff) |
| [#35](https://github.com/SiliconLabs/UnifySDK/pull/35) | Control over Size & Precision in Thermostat Setpoint CC | UIC-3320 | ZPC + ZAP/Dotdot regen | Hard |
| [#36](https://github.com/SiliconLabs/UnifySDK/pull/36) | Expose Z-Wave Generic and Specific Device Class | — | ZAP/Dotdot + Dev GUI | Hard |
| [#43](https://github.com/SiliconLabs/UnifySDK/pull/43) | User Credential CC support | — | ZPC + Dev GUI + ZAP (DRAFT) | Very hard |
| [#46](https://github.com/SiliconLabs/UnifySDK/pull/46) | Humidity Control CC | UIC-3042 | Partially in 1.6.0 release | Review before porting |

PRs with large ZAP-generated file changes should be ported as **standalone
releases**, not mixed with unrelated fixes.

---

## What changed in Unify 1.7.0 (not portable as-is)

Between `ver_1.6.0` and `ver_1.7.0`, ZPC was **removed** from UnifySDK
(~1142 files deleted under `applications/zpc/`). There are **no incremental
ZPC fix commits** in UnifySDK after 1.6.0 — only removal.

Post-1.6.0 ZPC development continues in the external
**z-wave-protocol-controller** repository (releases such as `ver_1.7.1`,
`ver_1.8.0`). To port those fixes:

1. Identify the fix in the external repo (release notes or commit).
2. Map the change to the equivalent path under `applications/zpc/` on
   `release/1.6.x`.
3. Apply manually and run ZPC unit tests (`ninja test` with `BUILD_ZPC=ON`).

Do **not** merge `main` or replace in-repo ZPC with the external repo without
a full migration plan.

---

## Recommended port order for `release/1.6.x`

1. **UIC-3335 / PR #47** — TX queue fail-safe (small, high impact).
2. **CVE-2025-10933 fix** — from external ZPC `ver_1.7.1+` (security).
3. **PR #30** — supervision flag (if supervision issues seen in production).
4. **Thermostat / humidity / user credential PRs** — only if required by product;
   expect large diffs and ZAP regen.
5. **Other 1.7.0+ external ZPC fixes** — case-by-case from external release notes.

Tag each maintenance drop (`ver_1.6.1`, `ver_1.6.2`, …) and update
`applications/zpc/release_notes.md` per fix.

---

## How to verify a fix on `release/1.6.x`

```bash
git checkout release/1.6.x
git checkout -b bugfix/1.6.x/zpc-<ticket>

# After applying the fix, build with ZPC enabled
mkdir -p build && cd build
cmake -GNinja -DCMAKE_TOOLCHAIN_FILE=../cmake/arm64_debian.cmake \
      -DBUILD_ZPC=ON -DBUILD_TESTING=ON ..
ninja
ninja test

# Confirm version still correct before release
cat ../cmake/release-version.cmake
```

## Checking upstream PR status

```bash
gh pr view 47 --repo SiliconLabs/UnifySDK
gh api repos/SiliconLabs/UnifySDK/pulls/47/files
```

Replace `47` with the PR number from the tables above.
