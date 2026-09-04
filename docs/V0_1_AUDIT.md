# v0.1 Foundation Audit Gate

Status: hardening patch.

## Problems found in the first prototype and resolution

| Finding | Resolution |
|---|---|
| separate Rust runtime duplicated the native engine boundary | replaced with standalone portable C++20 frontend |
| `f64` source values silently narrowed to `f32` | numeric lexemes retained exactly; type/range belongs to schema/host |
| O(n²) string interning | hash-indexed average O(1) interner |
| unchecked string IDs could panic/index out of range | `Plan::string()` is bounds checked and returns `optional` |
| no semantic validation | host `CommandSchema` validates command/argument/operator/entity contracts |
| premature `VXS1` persistent bytecode | removed; compiled plans are disposable until cache ABI is mature |
| ARM64-only script compile check | ARMv7 + ARM64 NDK matrix with pointer-width validation |
| target check did not prove Android linkability | CI links a real Android `.so` smoke target |
| no 16 KB page-size gate | Android link target uses 16 KB ELF alignment and CI inspects it |
| only minimal tests | expanded parsing/schema/bounds/UTF-8/entity/string/stress tests |
| unknown string escapes silently accepted | rejected with stable diagnostics |
| malformed UTF-8 not explicitly rejected | strict UTF-8 validation before tokenization |
| unbounded input/plans | explicit source/token/string/transaction/command/argument/intern limits |
| source could grow into a hidden platform authority | portable-core scan bans platform/JNI/Vulkan/filesystem/process hooks |
| raw IDs lacked kind information | typed entity reference carries kind + u64 |
| language could diverge from engine commands | reflected-schema integration seam is now the semantic authority |
| coordinate conventions were not frozen | compatibility snapshot records Vortex RH +X/+Y/+Z, meters/radians, column-major, xyzw |

## Remaining intentionally deferred work

These are not foundation defects and should only be added with the corresponding engine capability:

- symbolic/query bindings for portable macros;
- actual engine command catalog;
- transforms/vectors/quaternions as language values;
- compiled-plan persistent cache format;
- cancellation/progress execution adapter;
- dependency-derived job scheduling;
- editor Tree-sitter grammar.

The rule is that no deferred item gets a fake implementation merely to make a checklist look complete.
