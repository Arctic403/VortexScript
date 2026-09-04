#include "vortex_script/compiler.hpp"
#include "vortex_script/engine_contract.hpp"
#include "vortex_script/limits.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>

namespace {
using namespace vortex_script;

CommandSchema fixtureSchema() {
    CommandSchema schema;
    schema.commands = {
        CommandSpec{"asset_import", {
            ArgumentSpec{"asset", ValueType::String, true, ComparisonMask::Set, false, EntityKind::Object},
        }},
        CommandSpec{"mesh_budget", {
            ArgumentSpec{"mesh", ValueType::EntityRef, true, ComparisonMask::Set, true, EntityKind::Mesh},
            ArgumentSpec{"triangles", ValueType::Number, true, ComparisonMask::LessEqual, false, EntityKind::Object},
            ArgumentSpec{"background", ValueType::Bool, false, ComparisonMask::Set, false, EntityKind::Object},
        }},
    };
    return schema;
}

void testValidSchemaCompile() {
    const auto schema = fixtureSchema();
    const std::string source = R"(
transaction MobileAsset {
  command asset_import { asset "tree.glb" }
  command mesh_budget {
    mesh mesh:42
    triangles <= 20000
    background true
  }
}
)";
    const auto result = compile(source, &schema);
    assert(result.ok());
    assert(result.plan.transactions.size() == 1);
    assert(result.plan.transactions[0].commands.size() == 2);
}

void testExactNumberPreserved() {
    const auto result = compile("transaction T { command c { n 0.123456789012345678901234567890 } }");
    assert(result.ok());
    const auto& value = result.plan.transactions[0].commands[0].arguments[0].value;
    const auto number = std::get<NumberText>(value);
    const auto text = result.plan.string(number.text);
    assert(text.has_value());
    assert(*text == "0.123456789012345678901234567890");
}

void testUtf8AndEscapes() {
    const auto good = compile("transaction T { command c { label \"caf\xC3\xA9\" } }");
    assert(good.ok());
    std::string badUtf8 = "transaction T { command c { label \"";
    badUtf8.push_back(static_cast<char>(0xC3));
    badUtf8.push_back(static_cast<char>(0x28));
    badUtf8 += "\" } }";
    const auto bad = compile(badUtf8);
    assert(!bad.ok());
    assert(bad.diagnostics.front().code == DiagnosticCode::InvalidUtf8);
    const auto escape = compile(R"(transaction T { command c { label "\q" } })");
    assert(!escape.ok());
    assert(escape.diagnostics.front().code == DiagnosticCode::InvalidEscape);
}

void testSchemaErrors() {
    const auto schema = fixtureSchema();
    auto result = compile("transaction T { command nope {} }", &schema);
    assert(!result.ok() && result.diagnostics.front().code == DiagnosticCode::UnknownCommand);

    result = compile("transaction T { command asset_import { asset \"a\" asset \"b\" } }", &schema);
    assert(!result.ok() && result.diagnostics.front().code == DiagnosticCode::DuplicateArgument);

    result = compile("transaction T { command mesh_budget { mesh object:4 triangles <= 20 } }", &schema);
    assert(!result.ok() && result.diagnostics.front().code == DiagnosticCode::TypeMismatch);

    result = compile("transaction T { command mesh_budget { mesh mesh:4 } }", &schema);
    assert(!result.ok() && result.diagnostics.front().code == DiagnosticCode::MissingArgument);

    result = compile("transaction T { command mesh_budget { mesh mesh:4 triangles = 20 } }", &schema);
    assert(!result.ok() && result.diagnostics.front().code == DiagnosticCode::InvalidComparison);
}

void testEntityRefs() {
    auto result = compile("transaction T { command c { mesh mesh:18446744073709551615 } }");
    assert(result.ok());
    const auto entity = std::get<EntityRefValue>(result.plan.transactions[0].commands[0].arguments[0].value);
    assert(entity.kind == EntityKind::Mesh);
    assert(entity.id == UINT64_MAX);

    result = compile("transaction T { command c { mesh mesh:0 } }");
    assert(!result.ok() && result.diagnostics.front().code == DiagnosticCode::InvalidEntityRef);
}

void testSafeStringLookup() {
    const auto result = compile("transaction T { command c {} }");
    assert(result.ok());
    assert(!result.plan.string(UINT32_MAX).has_value());
}

void testBounds() {
    std::string tooLarge(limits::kMaxSourceBytes + 1, 'a');
    const auto result = compile(tooLarge);
    assert(!result.ok());
    assert(result.diagnostics.front().code == DiagnosticCode::SourceTooLarge);
}

void testDeterministicMalformedStress() {
    std::mt19937_64 rng(0x5658535F41554449ULL);
    static constexpr char alphabet[] = "{}:=<;_/\\\" abcdefghijklmnopqrstuvwxyz0123456789\n\t";
    for (int caseIndex = 0; caseIndex < 10000; ++caseIndex) {
        const std::size_t length = static_cast<std::size_t>(rng() % 256U);
        std::string source;
        source.reserve(length);
        for (std::size_t i = 0; i < length; ++i) {
            source.push_back(alphabet[rng() % (sizeof(alphabet) - 1U)]);
        }
        const auto first = compile(source);
        const auto second = compile(source);
        assert(first.ok() == second.ok());
        if (!first.ok()) {
            assert(first.diagnostics.front().code == second.diagnostics.front().code);
            assert(first.diagnostics.front().span.start == second.diagnostics.front().span.start);
            assert(first.diagnostics.front().span.end == second.diagnostics.front().span.end);
        }
    }
}

void testContractSnapshot() {
    static_assert(kPersistentIdBits == 64);
    static_assert(kInvalidPersistentId == 0);
    assert(kCoordinateContract.handedness == "right");
    assert(kCoordinateContract.rightAxis == "+X");
    assert(kCoordinateContract.forwardAxis == "+Y");
    assert(kCoordinateContract.upAxis == "+Z");
    assert(kCoordinateContract.linearUnit == "meter");
    assert(kCoordinateContract.angleUnit == "radian");
    assert(kCoordinateContract.matrixStorage == "column-major");
    assert(kCoordinateContract.vectorConvention == "column-vector");
    assert(kCoordinateContract.quaternionStorage == "xyzw");
}
}

int main() {
    testValidSchemaCompile();
    testExactNumberPreserved();
    testUtf8AndEscapes();
    testSchemaErrors();
    testEntityRefs();
    testSafeStringLookup();
    testBounds();
    testDeterministicMalformedStress();
    testContractSnapshot();
    std::cout << "VortexScript foundation tests passed\n";
    return 0;
}
