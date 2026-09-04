# VortexScript research notes

Research snapshot: September 2026.

This file records the external constraints behind the architecture so performance decisions do not turn into folklore later.

## Android native boundary

Android's JNI documentation says to minimize the amount of marshalling across JNI and the frequency of JNI crossings. It also recommends keeping the JNI layer small and concentrated. That strongly favors VortexScript submitting compiled plans or large jobs rather than making one managed/native call per mesh operation.

Source: https://developer.android.com/ndk/guides/jni-tips

**Decision:** Kotlin is the UI/lifecycle host. VortexScript compile/execute and large 3D data live on the native side. Cross the boundary with handles, direct/native buffers, commands, and coarse progress updates.

## Vulkan on Android

Android documents Vulkan as its primary low-level graphics API and says it provides optimal performance for games/custom engines that implement their own renderer.

Source: https://developer.android.com/games/develop/vulkan/native-engine-support

**Decision:** make Vulkan the primary APK backend. Keep VortexScript renderer-neutral so a web backend can target WebGPU and fallback paths can exist without changing scripts.

## Precision and memory bandwidth

Android's Vulkan guidance notes that appropriate reduced-precision formats can improve cache efficiency, memory bandwidth, power use, throughput, and RAM use. It also warns that half precision is not automatically faster and capability/algorithm behavior must be measured.

Source: https://developer.android.com/games/optimize/vulkan-reduced-precision

**Decision:** `precision half` is a policy hint, never a blind source-level guarantee. The Android backend decides whether a resource/kernel is safe and profitable to lower to 16-bit.

## Sustained mobile performance

Android's game optimization guidance exposes ADPF/performance hints, thermal APIs, memory advice, Game Mode, and profiling tools specifically because peak performance is not sustainable performance on mobile.

Sources:
- https://developer.android.com/games/optimize/overview
- https://developer.android.com/games/optimize/power

**Decision:** job scheduling and preview quality will eventually accept a device policy. Heavy geometry/texture jobs should reduce concurrency or quality under thermal pressure instead of forcing throttling.

## Frame pacing

Android's Frame Pacing library (Swappy) supports OpenGL and Vulkan and is designed to prevent buffer stuffing, inconsistent frame timing, and unnecessary display updates.

Source: https://developer.android.com/games/sdk/frame-pacing/

**Decision:** viewport presentation belongs in the Android renderer backend. It should not leak into normal VortexScript asset code.

## Rust targets

The Rust project currently lists both `aarch64-linux-android` and `wasm32-unknown-unknown` with standard-library support. This makes a single Rust compiler/runtime core practical for the APK and PWA paths.

Source: https://doc.rust-lang.org/rustc/platform-support.html

**Decision:** Rust owns the portable compiler/runtime foundation. Android gets a thin C/JNI bridge; web gets a WASM adapter.

## Why not a dynamic embedded VM first?

Wren is a useful comparison: it is an embeddable bytecode language with a fast host API, but its own documentation calls out the complexity introduced by dynamic typing and garbage-collected values crossing a native boundary. Wren also explains why bytecode VMs avoid the poor production performance of tree-walk interpreters.

Sources:
- https://wren.io/embedding/
- https://wren.io/performance.html

**Decision:** VortexScript will borrow the good idea—compile once to compact operations—but avoid a general dynamic object model and scripting GC in the hot path. Most expensive work is expressed as native jobs.

## Parser strategy

Tree-sitter is optimized for incremental parsing and can update syntax trees efficiently as text changes. Its runtime is designed to be embedded and robust under syntax errors.

Source: https://tree-sitter.github.io/

**Decision:** keep the tiny hand-written parser in the compiler core for now. Add a Tree-sitter grammar later for the Vortex3D code editor, syntax highlighting, incremental diagnostics, and navigation. The editor parser does not need to become the compiler parser.

## Resulting architecture

```text
                    Vortex3D UI / AI
                          |
                 VortexScript source
                          |
                 compiler (Rust core)
                          |
               typed IR / job graph
                          |
               compact cached plan
                    /           \
                   /             \
        Android native         Web WASM
          job runtime           adapter
              |                   |
      Vulkan + CPU pools        WebGPU
```

The central optimization is not syntax. It is that high-level intent becomes a compact, analyzable job plan before execution.
