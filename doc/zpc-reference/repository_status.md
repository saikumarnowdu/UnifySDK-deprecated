# Repository Status {#zpc_repository_status}

This page documents where ZPC source code lives across Unify SDK releases.

## Availability by Tag

| Branch / Tag | `applications/zpc/` | Notes |
|--------------|---------------------|-------|
| `main` (current) | **Absent** | Removed in `ver_1.7.0` |
| `ver_1.6.0` | **Present** | Full tree (~1,142 files) — last in-repo release |
| `ver_1.7.0` | **Absent** | Extracted to standalone repository |

## Migration (ver_1.7.0+)

From `ver_1.7.0`, ZPC was moved out of the Unify SDK monorepo:

- **GitHub:** https://github.com/SiliconLabsSoftware/z-wave-protocol-controller
- **Documentation:** https://siliconlabssoftware.github.io/z-wave-protocol-controller

The in-repo stub at `doc/protocol/zwave/zpc_introduction.md` points to these
external resources.

## Accessing Historical Source

To inspect or build ZPC from the last in-repo version:

```bash
git checkout ver_1.6.0 -- applications/zpc/
```

Or check out the full tag:

```bash
git checkout ver_1.6.0
```

## CMake Build Flag

In releases that include ZPC, the CMake option is:

```cmake
option(BUILD_ZPC "Package the ZPC" ON)
```

When `BUILD_ZPC=ON`, `applications/CMakeLists.txt` adds the `zpc` subdirectory.

## Doxygen Targets

| Target | Source | Status on `main` |
|--------|--------|------------------|
| `doxygen_zpc_reference` | `doc/zpc-reference/` | Always available |
| `doxygen_zpc` | `applications/zpc/components/` | Requires checked-out ZPC source |

## Related Pages

- [Overview](@ref zpc_overview)
- [Build and Deployment](@ref zpc_build_deploy)
