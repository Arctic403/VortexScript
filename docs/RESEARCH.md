# Research Record

Snapshot: 2026-09-04.

This file records constraints used by the foundation patch. External projects are references only; VortexScript remains independently implemented and MIT licensed.

## Vortex3D integration reference

`Arctic403/Vortex3d` is treated as read-only. VortexScript copies only compatibility contracts needed to stay integration-ready:

- portable C++20 engine boundary;
- stable typed 64-bit persistent IDs;
- command/transaction mutation path;
- Android ARMv7 + ARM64 support;
- scripts/automation/AI eventually sharing the reflected command system.

VortexScript does not modify or require that repository.

## Android native boundary

Android JNI guidance recommends minimizing marshalling and the number of managed/native crossings. VortexScript therefore produces coarse validated plans instead of encouraging one JNI call per low-level mesh operation.

Source: https://developer.android.com/ndk/guides/jni-tips

## Android 16 KB pages

Android 15 introduced 16 KB page-size devices. Current Android guidance says native apps should rebuild for 16 KB support; NDK r28+ uses 16 KB ELF alignment by default. Google Play requires 16 KB compatibility for apps targeting Android 15+ on 64-bit devices, with enforcement for updates tightening in 2027.

Source: https://developer.android.com/guide/practices/page-sizes

VortexScript CI therefore links an Android `.so` smoke library and inspects ELF `LOAD` alignment rather than treating target compilation alone as sufficient evidence.

## Android target API

Google Play's current policy requires new apps and updates to target Android 16 (API 36) or higher from August 31, 2026. VortexScript itself is a native library, so target-SDK selection belongs to the APK host; the library's NDK compile API level is a separate minimum-platform decision.

Source: https://developer.android.com/google/play/requirements/target-sdk

## glTF

glTF 2.0 defines a right-handed system with +Y up, +Z forward, -X right, meters, and radians. It is an interchange/runtime format, not VortexScript's native authoring coordinate contract.

Source: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html

## Parser/runtime choice

The compiler stays dependency-free and bounded. No parser generator, GC VM, JIT, filesystem capability, or general-purpose scripting runtime is required for the foundation. Editor incremental parsing can later be implemented separately without changing the authoritative compiler parser.
