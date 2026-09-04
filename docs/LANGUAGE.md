# VortexScript v0.1 Syntax Contract

The v0.1 syntax is a command-plan foundation. It is intentionally small.

```ebnf
program       = { transaction } EOF ;
transaction   = "transaction" identifier "{" { command } "}" ;
command       = "command" identifier "{" { argument } "}" ;
argument      = identifier [ comparison ] literal [ ";" ] ;
comparison    = "=" | "<=" ;
literal       = number | string | "true" | "false" | identifier | entity_ref ;
entity_ref    = entity_kind ":" unsigned_u64 ;
```

Entity kinds currently recognized by the compatibility contract are:

`document`, `scene`, `collection`, `object`, `mesh`, `material`, `image`, `vertex`, `edge`, `face`, `corner`.

## Numeric literals

Numeric spelling is preserved exactly through the plan. The compiler does not silently parse everything as `double` and narrow it to `float`. Host command schemas decide the eventual numeric type/range.

Supported syntax includes decimal integer/fraction forms and scientific notation. Entity IDs are stricter: decimal, non-zero, unsigned 64-bit integers only.

## Strings

Source is strict UTF-8. Strings support `\\`, `\"`, `\n`, `\r`, and `\t`. Unknown escapes are errors rather than silently changing meaning.

## Comments

`//` comments run to the end of the line.

## Comparison meaning

The parser preserves three forms:

- `value 4` -> Set
- `value = 4` -> Equal constraint
- `value <= 4` -> LessEqual constraint

Whether a form is legal is decided by the command schema.
