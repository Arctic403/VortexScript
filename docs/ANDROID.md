# Android Contract

VortexScript is a portable native library. Android-specific lifecycle, storage, rendering, permissions, and UI remain outside this core.

## ABI gate

CI cross-compiles for:

- `armeabi-v7a` with 32-bit pointer validation;
- `arm64-v8a` with 64-bit pointer validation.

The library uses fixed-width persistent IDs and does not serialize `size_t`, pointers, or addresses.

## 16 KB page sizes

Android supports 16 KB page-size devices. Native apps must not assume 4096-byte pages. The portable-core scanner rejects `PAGE_SIZE` and `sys/mman.h` use.

Android CI links a real shared-library smoke target with 16 KB ELF linker alignment and inspects its load-segment alignment. This is stronger than merely running a target compile against an Android triple.

Final APK/AAB packaging remains an application-host responsibility because VortexScript does not build the Android app package.

## JNI boundary

VortexScript itself needs no JNI API. A future Android host should submit source/compiled plans through coarse calls and keep large geometry buffers inside the native engine rather than crossing JNI per vertex/face operation.
