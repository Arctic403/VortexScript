# VortexScript

VortexScript is an Android-first 3D domain-specific language for Vortex3D.

The goal is not to replace Rust, Kotlin, C++, or JavaScript. VortexScript describes 3D work at a high level, compiles it into a compact execution plan, and lets a platform backend perform the expensive work without bouncing thousands of tiny calls across platform boundaries.

## Why this architecture

- **Android first:** compile whole jobs, then execute them inside the native runtime.
- **One core:** the compiler/runtime is Rust so the same core can target Android native and `wasm32-unknown-unknown` for the PWA.
- **No GC in the language core:** predictable allocations and no scripting garbage collector in frame-sensitive work.
- **Typed 3D vocabulary:** models, materials, optimization budgets, LODs, textures, and future mesh operations become language concepts instead of generic dynamic objects.
- **Compact plans:** source text is parsed once; execution uses interned strings and compact operations.
- **Backend neutral:** Vulkan/Android and WebGPU/web backends can consume the same compiled plan.

## Foundation syntax

```vortex
model Tree {
    import "tree.glb"

    optimize android {
        triangles <= 20000
        texture <= 1024
        lod 3
        precision half
    }

    material Bark {
        roughness 0.8
        metallic 0.0
    }
}
```

The current foundation implements:

- lexer with source spans and diagnostics
- parser for `model`, `import`, `optimize`, and `material`
- literals: numbers, strings, booleans, and identifiers
- `=`, `<=`, and concise set syntax
- interned-string intermediate representation
- deterministic binary plan encoding (`VXS1`)
- backend execution interface
- tiny CLI for checking and compiling `.vxs` files
- native-friendly Rust core with a WASM CI target

## CLI

```bash
cargo run -p vortex-script -- check examples/android_asset.vxs
cargo run -p vortex-script -- compile examples/android_asset.vxs
```

The second command writes `examples/android_asset.vxb` unless an output path is supplied.

## Direction

VortexScript source should stay ergonomic while the runtime gets increasingly data-oriented. The intended Android path is:

`VortexScript -> parser -> typed/validated IR -> execution plan -> native job system -> Vulkan/CPU workers`

The Kotlin UI should submit large jobs instead of individual mesh operations. This keeps JNI traffic low and leaves geometry buffers on the native side.

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) for the design decisions and Android optimization plan.
