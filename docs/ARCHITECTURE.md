# VortexScript Architecture

VortexScript is a bounded automation frontend for Vortex3D. It is deliberately not a second modeling engine, renderer, VM, filesystem layer, or Android runtime.

## Pipeline

```text
.vxs source / AI-generated macro
            |
       lexer + parser
            |
      bounded AST
            |
 host CommandSchema validation
            |
      typed command plan
            |
      adapter boundary
            |
 Vortex commands + transactions
```

The repository is standalone. It does not include or link Vortex3D source. Integration is by contract: the host provides a reflected `CommandSchema` and translates a validated `Plan` into its existing command/transaction APIs.

## Non-negotiable rules

1. The compiler does not own document or mesh state.
2. The compiler cannot mutate Vortex3D directly.
3. No Android, JNI, Vulkan, Web, renderer, filesystem, network, process, or shell authority exists in the portable core.
4. Source parsing and lowering are bounded by explicit limits.
5. Untrusted source must fail with diagnostics, never unchecked indexing or undefined behavior.
6. User-visible persistent work remains source/host data; compiled plans are disposable.
7. Command semantics come from a host-provided schema rather than duplicated hand-written engine behavior.
8. Scripts and AI must use the same command/transaction surface as editor automation.
9. Parallelism is a host/evaluation decision derived from dependencies; source cannot demand unsafe parallel execution.
10. Raw pointers, native handles, Vulkan objects, JNI references, and filesystem paths are not persistent script values.

## Why C++20

The frontend is dependency-free C++20 so an Android/native Vortex host can link it directly without adding another managed/native runtime boundary. A future web adapter can compile the same portable frontend separately if needed.

## Entity references

The foundation supports typed 64-bit entity references such as `mesh:42`. Zero is invalid. The entity kind is retained in the plan instead of becoming a naked integer.

These numeric references are document-bound. A future portable macro/query layer must use symbolic bindings or queries rather than pretending raw document IDs are globally portable.

## Command schema

`CommandSchema` is the integration seam. Each command defines argument names, types, requiredness, allowed comparison operators, and optional entity-kind constraints.

The compiler rejects unknown commands, unknown/duplicate/missing arguments, mismatched types, invalid comparison operators, and entity-kind mismatches before a plan can reach the host.

## No stable bytecode yet

A stable binary instruction set is intentionally deferred until the engine command schema and cache versioning policy are mature. Freezing opcodes before semantics exist would manufacture compatibility debt.
