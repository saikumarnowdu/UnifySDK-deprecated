# ZPC v2.0.0 no-supervision SET / GET patch

Applies to [Silicon Labs Z-Wave Protocol Controller](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller) tag `zpc-v2.0.0`.

See also: [ZPC repository map](../README.md) for how this repo relates to UnifySDK, legacy ZPC, and v2.0.

ZPC v2.0.0 is a **new codebase** (not API-compatible with legacy Unify ZPC 1.6.x). Its v2.0.0 release notes list **no bug fixes** for this area; the generic attribute resolver still undefines `reported` on `RESOLVER_SEND_STATUS_OK`, which forces a redundant GET when the SET reply already updated state.

## Apply

```bash
git checkout -b cursor/binary-switch-no-get-03e8 zpc-v2.0.0
patch -p1 < tools/zpc_v2_supervision_no_get/0001-*.patch
```

## What it changes

| File | Change |
|------|--------|
| `components/attribute_resolver/src/attribute_resolver_rule.cpp` | Skip GET when group `reported` already matches `desired` on no-supervision SET OK |
| `components/attribute_resolver/src/zpc_attribute_resolver_callbacks.cpp` | Map matched SET TX complete to `EXECUTION_VERIFIED`; same for supervision NO_SUPPORT |
| `components/attribute_resolver/src/zpc_attribute_resolver_send.c` | `number_of_responses=1` on no-supervision SET so RX can update reported before TX complete |

## Legacy Unify ZPC (1.6.x)

Binary Switch also has a **command-class-specific** probe handler in `applications/zpc/.../zwave_command_class_binary_switch.c`. See branch `cursor/binary-switch-no-get-03e8` in UnifySDK for the 1.6s verify-then-decide logic (`ZWAVE_RECOMMENDED_RESPONSE_TIME_MS`).
