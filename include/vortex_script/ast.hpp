#pragma once

#include "vortex_script/diagnostic.hpp"
#include "vortex_script/engine_contract.hpp"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace vortex_script {

struct NumberLiteral final { std::string lexeme; };
struct StringLiteral final { std::string value; };
struct BoolLiteral final { bool value = false; };
struct IdentifierLiteral final { std::string value; };
struct EntityRefLiteral final { EntityKind kind = EntityKind::Object; std::uint64_t id = 0; };
using Literal = std::variant<NumberLiteral, StringLiteral, BoolLiteral, IdentifierLiteral, EntityRefLiteral>;

enum class CompareOp : std::uint8_t { Set, Equal, LessEqual };

struct Argument final {
    std::string name;
    CompareOp op = CompareOp::Set;
    Literal value;
    Span span{};
};

struct CommandDecl final {
    std::string name;
    std::vector<Argument> arguments;
    Span span{};
};

struct TransactionDecl final {
    std::string name;
    std::vector<CommandDecl> commands;
    Span span{};
};

struct Program final {
    std::vector<TransactionDecl> transactions;
};

} // namespace vortex_script
