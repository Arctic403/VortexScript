#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace vortex_script {

inline constexpr std::uint32_t kEngineContractVersion = 1;
inline constexpr std::uint32_t kPersistentIdBits = 64;
inline constexpr std::uint64_t kInvalidPersistentId = 0;

struct CoordinateContract final {
    std::string_view id = "vortex-rh-y-forward-z-up";
    std::string_view handedness = "right";
    std::string_view rightAxis = "+X";
    std::string_view forwardAxis = "+Y";
    std::string_view upAxis = "+Z";
    std::string_view linearUnit = "meter";
    std::string_view angleUnit = "radian";
    std::string_view matrixStorage = "column-major";
    std::string_view vectorConvention = "column-vector";
    std::string_view quaternionStorage = "xyzw";
};

inline constexpr CoordinateContract kCoordinateContract{};

enum class EntityKind : std::uint8_t {
    Document,
    Scene,
    Collection,
    Object,
    Mesh,
    Material,
    Image,
    Vertex,
    Edge,
    Face,
    Corner,
};

[[nodiscard]] constexpr std::string_view entityKindName(const EntityKind kind) noexcept {
    switch (kind) {
        case EntityKind::Document: return "document";
        case EntityKind::Scene: return "scene";
        case EntityKind::Collection: return "collection";
        case EntityKind::Object: return "object";
        case EntityKind::Mesh: return "mesh";
        case EntityKind::Material: return "material";
        case EntityKind::Image: return "image";
        case EntityKind::Vertex: return "vertex";
        case EntityKind::Edge: return "edge";
        case EntityKind::Face: return "face";
        case EntityKind::Corner: return "corner";
    }
    return "unknown";
}

[[nodiscard]] constexpr bool parseEntityKind(const std::string_view text, EntityKind& out) noexcept {
    if (text == "document") out = EntityKind::Document;
    else if (text == "scene") out = EntityKind::Scene;
    else if (text == "collection") out = EntityKind::Collection;
    else if (text == "object") out = EntityKind::Object;
    else if (text == "mesh") out = EntityKind::Mesh;
    else if (text == "material") out = EntityKind::Material;
    else if (text == "image") out = EntityKind::Image;
    else if (text == "vertex") out = EntityKind::Vertex;
    else if (text == "edge") out = EntityKind::Edge;
    else if (text == "face") out = EntityKind::Face;
    else if (text == "corner") out = EntityKind::Corner;
    else return false;
    return true;
}

enum class ValueType : std::uint8_t {
    Number,
    String,
    Bool,
    Identifier,
    EntityRef,
};

enum class ComparisonMask : std::uint8_t {
    None = 0,
    Set = 1U << 0U,
    Equal = 1U << 1U,
    LessEqual = 1U << 2U,
};

[[nodiscard]] constexpr ComparisonMask operator|(const ComparisonMask a, const ComparisonMask b) noexcept {
    return static_cast<ComparisonMask>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}

[[nodiscard]] constexpr bool contains(const ComparisonMask mask, const ComparisonMask flag) noexcept {
    return (static_cast<std::uint8_t>(mask) & static_cast<std::uint8_t>(flag)) != 0;
}

struct ArgumentSpec final {
    std::string name;
    ValueType type = ValueType::Identifier;
    bool required = false;
    ComparisonMask comparisons = ComparisonMask::Set;
    bool constrainEntityKind = false;
    EntityKind entityKind = EntityKind::Object;
};

struct CommandSpec final {
    std::string name;
    std::vector<ArgumentSpec> arguments;
};

struct CommandSchema final {
    std::uint32_t contractVersion = kEngineContractVersion;
    std::vector<CommandSpec> commands;

    [[nodiscard]] const CommandSpec* findCommand(const std::string_view name) const noexcept {
        for (const auto& command : commands) {
            if (command.name == name) return &command;
        }
        return nullptr;
    }
};

} // namespace vortex_script
