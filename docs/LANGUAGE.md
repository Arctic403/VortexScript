# VortexScript language foundation

This document freezes the small v0.1 grammar so future features can grow without silently breaking old scripts.

## File extension

Source: `.vxs`

Compiled plan: `.vxb`

## Lexical rules

- UTF-8 source files.
- Identifiers currently use ASCII letters, digits, and `_`; the first character cannot be a digit.
- Strings are quoted with `"` and support `\\`, `\"`, `\n`, and `\t` escapes.
- Numbers are decimal integers or floats and may be negative.
- `//` starts a line comment.
- Semicolons are optional after import and parameter statements.
- Keywords are lexed as identifiers and interpreted by the parser. This keeps the lexer small.

## v0.1 grammar

```ebnf
program          = { model_decl } EOF ;

model_decl       = "model" identifier "{" { model_statement } "}" ;

model_statement  = import_statement
                 | optimize_block
                 | material_block ;

import_statement = "import" string [ ";" ] ;

optimize_block   = "optimize" identifier parameter_block ;
material_block   = "material" identifier parameter_block ;

parameter_block  = "{" { parameter } "}" ;
parameter        = identifier [ comparison ] literal [ ";" ] ;
comparison       = "=" | "<=" ;

literal          = number
                 | string
                 | "true"
                 | "false"
                 | identifier ;
```

## Meaning of concise set syntax

These are equivalent at the AST level except for the comparison code:

```vortex
lod 3
lod = 3
triangles <= 20000
```

`lod 3` is a direct property/configuration assignment. `=` is an explicit equality request. `<=` is a budget/constraint request. Backends are expected to preserve that distinction.

## Compatibility rules

- New statement kinds may be added in minor versions.
- Existing opcode meanings must not change.
- `.vxb` binary changes require a format version bump.
- Unknown future optimization parameters should eventually be preserved through semantic analysis so newer backends can use them.
- Platform-specific objects such as Vulkan handles, JNI references, DOM nodes, and WebGPU objects are never language values.

## Planned syntax families

These are design targets, not implemented grammar yet:

```vortex
mesh Body {
    decimate ratio 0.65
    normals recalc
    lod auto 4
}

transform Body {
    position vec3(0, 1, 0)
    rotation euler(0, 90, 0)
    scale 1.0
}

job MobileExport {
    parallel {
        generate_lods Body
        bake_collision Body
        compress_textures android
    }
}
```

The implementation should only add syntax when it maps to a useful compiler/runtime concept. VortexScript should not grow general-purpose features simply because other languages have them.
