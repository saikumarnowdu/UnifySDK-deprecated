# Maintaining the Unify SDK 1.6.x Product Line

This guide describes how to maintain a long-lived **1.6.x** release branch
while `main` tracks the latest Unify SDK (currently **1.7.0**). It also
explains how to port selected fixes and features from newer releases into
the 1.6.x line.

For general contribution conventions (branch naming, commit messages, pull
requests, and unit tests), see [CONTRIBUTING.md](../CONTRIBUTING.md).

## Overview

The Unify SDK uses **linear, single-version releases**. Each checkout or
build corresponds to one SDK version (for example `ver_1.6.0`). There is
no in-tree feature flag or version matrix to target multiple SDK versions
from a single branch.

| Branch | Purpose | Current version |
| ------ | ------- | --------------- |
| `main` | Latest upstream development | 1.7.0 (`ver_1.7.0`) |
| `release/1.6.x` | Long-lived 1.6.x product line | 1.6.0+ (`ver_1.6.x`) |

**Compatibility rule:** All components in one deployment must use the
**same Unify SDK version**. Do not mix 1.6.x packages with 1.7.0 packages
in the same network.

## What is different on `release/1.6.x`

The 1.6.x line is based on the official `ver_1.6.0` tag (commit
`954fdce5d`). Compared to `main` (1.7.0), the 1.6.x branch still includes
components that were removed or moved in 1.7.0:

| Component | 1.6.x (`release/1.6.x`) | 1.7.0 (`main`) |
| --------- | ----------------------- | -------------- |
| ZPC | In-repo (`applications/zpc/`) | Separate [z-wave-protocol-controller](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller) repo |
| ZigPC | Included | Removed |
| Zigbeed | Included | Removed |
| AoXPC | Included | Removed |
| home_assistant | Included | Removed |
| EED | Portable runtime support | Full EED application in SDK |

If your product depends on in-repo ZPC or other removed components, use
`release/1.6.x` — not `main`.

## Branch model

```
release/1.6.x              ← stable maintenance line (merge target)
  ├── bugfix/1.6.x/<ticket>-<description>
  └── feature/1.6.x/<feature-name>
```

| Branch prefix | Use case | Example |
| ------------- | -------- | ------- |
| `bugfix/1.6.x/` | Targeted fixes for the 1.6.x line | `bugfix/1.6.x/GH-1234-gms-reconnect-crash` |
| `feature/1.6.x/` | Ports from newer releases or new 1.6.x features | `feature/1.6.x/port-dotdot-on-off-fix` |

**Do not merge `main` into `release/1.6.x`.** That would pull in 1.7.0
removals (ZPC, ZigPC, AoXPC, and others) and break the 1.6.x product line.

## Version numbering

Version is defined in `cmake/release-version.cmake`:

```cmake
SET(GIT_VERSION "ver_1.6.0")
SET(GIT_VERSION_SHA "f0d8a05d")
```

CMake parses `ver_X.Y.Z` and propagates it to:

- Build artifacts and Debian package names (for example `uic-gms_1.6.1_arm64.deb`)
- Generated headers (`UIC_VERSION`, `UIC_VERSION_MAJOR`, etc.)
- Runtime and MQTT-reported application versions
- Dev GUI version display

For maintenance releases, bump the **patch** (revision) number:

| Release | `GIT_VERSION` |
| ------- | ------------- |
| Initial 1.6.0 | `ver_1.6.0` |
| First maintenance drop | `ver_1.6.1` |
| Second maintenance drop | `ver_1.6.2` |

Always update `GIT_VERSION_SHA` to the short commit SHA of the release commit.

## Getting started on the maintenance branch

```bash
git fetch origin --tags
git checkout release/1.6.x

# Verify version
cat cmake/release-version.cmake
# Expected: SET(GIT_VERSION "ver_1.6.0")
```

To create the branch from scratch (if it does not exist yet):

```bash
git checkout -b release/1.6.x ver_1.6.0
git push -u origin release/1.6.x
```

## Patch release checklist

Follow this checklist for every maintenance release (`ver_1.6.1`,
`ver_1.6.2`, and so on):

1. Merge reviewed pull requests into `release/1.6.x`.
2. Update `cmake/release-version.cmake` with the new version and SHA.
3. Add a `## [1.6.N]` section to `doc/release_notes.md`.
4. Update affected application `release_notes.md` files if those apps changed.
5. Build and publish packages (see [Build instructions](#build-instructions)).
6. Tag the release and push:

   ```bash
   git tag ver_1.6.N
   git push origin ver_1.6.N
   ```

7. Use the release commit message format: `Release ver_1.6.N`.

## Build instructions

Build on the 1.6.x branch the same way as documented in
[readme_building.md](readme_building.md):

```bash
./docker/build_docker.sh arm64 uic_arm64
docker run -it --rm -v $PWD:$PWD -w $PWD uic_arm64
mkdir -p build && cd build
cmake -GNinja -DCMAKE_TOOLCHAIN_FILE=../cmake/arm64_debian.cmake ..
ninja
ninja deb   # produces unify_1.6.N_arm64.zip
```

Ship the resulting `.deb` packages as a **matched set** from a single build.

## Continuous integration

Configure CI to run on `release/1.6.x` with the full 1.6.0 component set
(ZPC, ZigPC, AoXPC, and others). Builds on `main` (1.7.0) do **not**
validate the 1.6.x line because large parts of that tree were removed in
1.7.0.

Before raising a pull request, run unit tests locally:

```bash
cmake -GNinja -DBUILD_TESTING=ON ...
ninja
ninja test
```

See [CONTRIBUTING.md](../CONTRIBUTING.md) for full unit test and PR guidelines.

## Porting features from 1.7.0

Port features **after** the 1.6.x maintenance line is stable and you have
a patch release process in place.

Most of the 1.6.0 → 1.7.0 delta landed in a single large release commit.
Cherry-picking entire commits from `main` rarely works. Port **by feature
area or file path**, not by merging `main`.

### Workflow per feature

```bash
git checkout release/1.6.x
git checkout -b feature/1.6.x/port-<feature-name>

# Inspect what changed in 1.7.0 for your area
git diff ver_1.6.0..ver_1.7.0 -- <paths>

# Optional: try cherry-pick for small, isolated commits only
git cherry-pick <commit-sha>

# Build and test on the 1.6.x stack
ninja
ninja test
```

Open a pull request targeting `release/1.6.x`. After merge, include the
change in the next patch release.

### What ports cleanly

| Difficulty | Category | Examples | Approach |
| ---------- | -------- | -------- | -------- |
| Easy | Shared library bug fixes | Dotdot, MQTT, attribute store, JSON helpers | File-level diff from `ver_1.6.0..ver_1.7.0` |
| Easy | IoT service fixes | GMS, NAL, UPVL, Dev GUI | Same as above if the app exists on 1.6.x |
| Easy | Small build or tooling fixes | Dev GUI yarn/npm changes | Cherry-pick individual commits |
| Manual | ZPC fixes | Fixes in external ZPC repo (1.7.0+) | Port hunks manually into `applications/zpc/` |
| Hard | New 1.7.0 architecture | Full EED application in SDK, ZAP v2025 regen | Reimplement on 1.6.x base; evaluate cost first |
| Skip | Removed in 1.7.0 | ZigPC, AoXPC, in-repo ZPC removal | Already present on 1.6.x — no port needed |

### Decision tree

```
Does the component still exist on release/1.6.x?
├─ NO  → Skip, or reimplement separately on the 1.6.x base
└─ YES → Is it a bug fix or small enhancement?
         ├─ YES → Port via file-level diff or cherry-pick
         └─ NO  → Is it tied to 1.7.0-only architecture?
                  ├─ YES → Evaluate cost; often not worth porting
                  └─ NO  → Port incrementally with tests
```

### ZPC note

In 1.7.0, ZPC is maintained in a
[separate repository](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller).
On the 1.6.x line, continue using in-repo ZPC at `applications/zpc/`. To
bring in ZPC fixes from the external repo, port them manually — do not
replace in-repo ZPC with the external repo without a full migration plan
(that migration is effectively a move to 1.7.0+).

## Deployment options for 1.6.x

### Prebuilt packages

Download `unify_1.6.0_arm64.zip` (or your patched `unify_1.6.N_arm64.zip`)
from [GitHub Releases](https://github.com/SiliconLabs/UnifySDK/releases) and
install:

```bash
sudo apt install ./uic-<component>_1.6.N_arm64.deb
```

### Portable runtime

Download the matching portable runtime asset from releases, or replace
packages under `portable_runtime_<OS>/resources/docker-files/` with your
1.6.x `.deb` files. See
[portable_runtime/readme_user.md](portable_runtime/readme_user.md).

## Recommended order of work

1. **Establish the line:** Use `release/1.6.x` as the product branch; enable CI.
2. **Stabilize:** Land bug fixes only; tag `ver_1.6.1`, `ver_1.6.2`, and so on.
3. **Port features:** Maintain a backlog (ticket → target paths → port
   difficulty). Start with shared-component fixes; defer or skip
   architectural 1.7.0 changes.

## What not to do

| Approach | Supported? |
| -------- | ---------- |
| Run 1.6.x and 1.7.0 components in one deployment | No |
| Merge `main` into `release/1.6.x` | No |
| Use SDK version `#ifdef`s to target 1.6.x from `main` | No — not supported by the codebase |
| Mix 1.6.x `libunify` with 1.7.0 services | No |

## ZPC fixes (check before porting other features)

Before porting general 1.7.0 features, review the ZPC fix audit:

**[ZPC Fixes Audit for the 1.6.x Product Line](zpc_fixes_audit_1.6.x.md)**

That document lists which 1.6.0 ZPC fixes are already present, known open
issues (including UIC-3335 TX queue lock), security items, and recommended
port order from upstream PRs and the external z-wave-protocol-controller repo.

## Related documentation

- [CONTRIBUTING.md](../CONTRIBUTING.md) — branch naming, commits, PRs, tests
- [zpc_fixes_audit_1.6.x.md](zpc_fixes_audit_1.6.x.md) — ZPC fix status and port priorities
- [release_notes.md](release_notes.md) — shared component changelog
- [readme_building.md](readme_building.md) — build and package instructions
- [getting_started.md](getting_started.md) — installing `.deb` packages
