# ZPC supervision no-GET patch

Fixes unnecessary GET commands after SET on devices that do not support
Supervision but return the new value in the SET reply.

## Problem

When a node does not support Supervision, ZPC sends SET without supervision
encapsulation. Many devices reply to SET with a REPORT that already contains
the updated value. The resolver was still:

1. Completing the SET with `RESOLVER_SEND_STATUS_OK`
2. Undefining `reported` to force a follow-up GET
3. Sending a redundant GET and slowing the Z-Wave TX queue

## Apply

Against a tree that contains `applications/zpc` or the standalone Z-Wave
Protocol Controller repository:

```bash
patch -p1 < tools/zpc_supervision_no_get/0001-zpc-supervision-no-get.patch
```

Dry-run:

```bash
patch -p1 --dry-run < tools/zpc_supervision_no_get/0001-zpc-supervision-no-get.patch
```

## What it changes

| File | Change |
|------|--------|
| `zpc_attribute_resolver_callbacks.cpp` | Treat matched SET replies as verified; handle supervision NO_SUPPORT when reported already matches |
| `zpc_attribute_resolver_send.c` | Wait for one application response on no-supervision SET (`number_of_responses=1`) |

The shared UIC `attribute_resolver_rule.cpp` fix in this repository also skips
the GET path when `reported` already matches `desired` on `RESOLVER_SEND_STATUS_OK`.
