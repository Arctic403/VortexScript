# VortexScript architecture

## Mission

VortexScript is a domain-specific command language for Vortex3D. Its performance advantage should come from restricting the problem: the compiler understands 3D operations and can turn them into predictable jobs instead of executing arbitrary dynamic script behavior.

## Core pipeline

1. **Lex** source into tokens with byte spans.
2. **Parse** tokens into a small AST.
3. **Validate/type** Vortex concepts (next milestone).
4. **Lower** the AST into an interned execution plan.
5. **Encode/cache** the plan as deterministic `VXS1` bytecode when desired.
6. **Execute** the plan against a platform backend.

The source language is deliberately separated from backend APIs. A script should not contain Vulkan handles, JNI objects, DOM objects, or WebGPU objects.

## Android design rules

### 1. JNI is a control boundary, not a data pipeline

Android's JNI guidance recommends minimizing both marshalling and the frequency of crossings. Vortex3D should therefore submit a compiled plan or a small number of large native jobs, not vertices, faces, or modifier calls one-by-one.

Planned Android shape:

`Kotlin UI -> compile/submit -> native Vortex runtime -> worker queues -> Vulkan + native geometry kernels`

Large mesh/texture buffers should stay native once imported. Kotlin should exchange handles, status, progress, and small metadata.

Reference: https://developer.android.com/ndk/guides/jni-tips

### 2. Vulkan is the primary Android renderer target

Android currently documents Vulkan as the primary low-level graphics API and the optimal route for custom engines that need maximum rendering performance. VortexScript itself remains renderer-neutral; the Android backend maps render/compute jobs to Vulkan.

Reference: https://developer.android.com/games/develop/vulkan/native-engine-support

### 3. Optimize bandwidth, not only arithmetic

Mobile GPUs are strongly constrained by memory bandwidth and thermals. Vortex optimization profiles should be able to request reduced precision, compact vertex layouts, texture compression, LOD budgets, and transient-memory reuse.

Android specifically notes that appropriate 16-bit data can reduce memory bandwidth and RAM usage, while also warning that conversion overhead and device capabilities must be measured.

Reference: https://developer.android.com/games/optimize/vulkan-reduced-precision

### 4. Sustained performance beats benchmark spikes

Vortex3D is an editor, so a device can spend minutes doing heavy work. The Android backend should adapt job concurrency and preview quality using thermal/performance signals instead of pinning the CPU/GPU at maximum load until throttling occurs.

References:
- https://developer.android.com/games/optimize/overview
- https://developer.android.com/games/optimize/power

### 5. Frame pacing matters even in an editor

Interactive orbiting, sculpt previews, and viewport transforms need consistent frame delivery. Android's Frame Pacing library supports Vulkan and can reduce buffer stuffing/stutter. This belongs in the renderer backend, not in VortexScript syntax.

Reference: https://developer.android.com/games/sdk/frame-pacing/

## Runtime principles

- No language-level reflection in hot paths.
- No per-frame AST walking: parse once and lower once.
- No requirement for a language garbage collector.
- Strings are interned in plans.
- Expensive operations become explicit jobs.
- Backends own platform resources; scripts own logical handles only.
- Immutable compiled plans are cacheable and safe to send to workers.
- Device-specific optimization happens during lowering/backend execution, not by making source scripts device-specific everywhere.

## Planned type system

Foundation types:

- `bool`
- `i32`, `u32`, `f32`
- `vec2`, `vec3`, `vec4`
- `quat`, `mat4`
- `mesh`, `material`, `texture`, `model`
- typed resource handles

The compiler should prefer 32-bit values by default on Android. Half precision should be an explicit optimization hint that is only applied where the backend verifies device support and acceptable error.

## Planned execution model

VortexScript should lower high-level operations into a job graph. A future example:

```text
ImportMesh -> ValidateMesh -> GenerateTangents
                         -> BuildLODs
                         -> BuildCollision
                         -> UploadGPU
```

Independent jobs can run concurrently. The graph also gives Vortex3D a natural place for cancellation, progress reporting, caching, undo checkpoints, and AI-generated edit validation.

## What VortexScript should not become

- a general-purpose replacement for Rust/Kotlin/C++
- a JIT-heavy dynamic language
- a second renderer API
- an API where every triangle edit crosses JNI
- a syntax wrapper over giant untyped JSON objects

Those directions would erase most of the Android advantage.

## Milestones

### v0.1 foundation

- lexer/parser
- AST
- compact IR/plan
- binary encoder
- backend interface
- CLI
- Android/WASM build checks

### v0.2 semantics

- typed values and symbol table
- semantic diagnostics
- resource handles
- transform/mesh operation syntax
- stable bytecode decoder + versioning rules

### v0.3 Android bridge

- C ABI/JNI bridge
- direct/native buffers
- plan cache
- worker/job system
- cancellation and progress channel

### v0.4 graphics-aware optimization

- Vulkan capability profile
- LOD/mesh budgets
- texture profile (ASTC/ETC2 fallback)
- reduced-precision policy
- thermal-aware quality policy
- frame pacing hooks
