#pragma once

#include "vortex_script/ast.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace vortex_script {

using StringId = std::uint32_t;
struct NumberText final { StringId text = 0; };
struct StringValue final { StringId text = 0; };
struct IdentifierValue final { StringId text = 0; };
struct EntityRefValue final { EntityKind kind = EntityKind::Object; std::uint64_t id = 0; };
using PlannedValue = std::variant<NumberText, StringValue, bool, IdentifierValue, EntityRefValue>;

struct PlannedArgument final {
    StringId name = 0;
    CompareOp op = CompareOp::Set;
    PlannedValue value;
};

struct PlannedCommand final {
    StringId name = 0;
    std::vector<PlannedArgument> arguments;
};

struct PlannedTransaction final {
    StringId name = 0;
    std::vector<PlannedCommand> commands;
};

struct Plan final {
    std::uint32_t engineContractVersion = kEngineContractVersion;
    std::vector<std::string> strings;
    std::vector<PlannedTransaction> transactions;

    [[nodiscard]] std::optional<std::string_view> string(const StringId id) const noexcept {
        if (static_cast<std::size_t>(id) >= strings.size()) return std::nullopt;
        return strings[id];
    }
};

} // namespace vortex_script
