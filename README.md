# VortexScript

VortexScript is the bounded automation language frontend for Vortex3D.

This repository is **standalone**. It does not modify, include, or require the Vortex3D repository to build. VortexScript compiles source into a validated command plan; a host adapter later maps that plan onto Vortex's existing command/transaction system.

```text
VortexScript / AI macro
        |
 bounded C++20 frontend
        |
 host-reflected CommandSchema
        |
 validated command plan
        |
 host adapter -> Vortex commands/transactions
```

## Hardened v0.1 foundation

- dependency-free portable C++20 core;
- strict UTF-8 source handling;
- stable diagnostic codes + byte spans;
- exact numeric spelling (no silent float narrowing);
- typed 64-bit entity references;
- bounded source/tokens/strings/commands/arguments;
- average O(1) string interning;
- bounds-checked string lookup;
- schema-based semantic validation;
- deterministic lowering;
- no persistent bytecode compatibility debt yet;
- no filesystem/network/process/JNI/Vulkan authority in the compiler core;
- GCC + Clang warnings-as-errors;
- ASan + UBSan;
- deterministic 10,000-input malformed-source stress test;
- Android ARMv7 + ARM64 cross-compiles;
- real Android shared-library link smoke and 16 KB ELF-alignment gate.

## Syntax

```vortex
transaction MobileAsset {
    command asset_import {
        asset "tree.glb"
    }

    command mesh_budget {
        mesh mesh:42
        triangles <= 20000
        background true
    }
}
```

The command names above are fixture examples. Production command names/types are supplied by the host's reflected schema instead of being duplicated inside VortexScript.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DVORTEX_SCRIPT_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/vortex-script check examples/android_asset.vxs
```

Read `docs/V0_1_AUDIT.md` before expanding semantics.
