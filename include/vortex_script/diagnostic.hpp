#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace vortex_script {

struct Span final {
    std::uint32_t start = 0;
    std::uint32_t end = 0;
};

enum class DiagnosticCode : std::uint16_t {
    SourceTooLarge,
    InvalidUtf8,
    TokenLimit,
    IdentifierTooLong,
    StringTooLong,
    NumberTooLong,
    UnexpectedCharacter,
    UnterminatedString,
    InvalidEscape,
    InvalidNumber,
    ExpectedToken,
    ExpectedIdentifier,
    ExpectedLiteral,
    LimitExceeded,
    DuplicateArgument,
    UnknownCommand,
    UnknownArgument,
    MissingArgument,
    TypeMismatch,
    InvalidComparison,
    InvalidEntityRef,
    SchemaVersionMismatch,
    InvalidSchema,
    InvalidPlan,
};

[[nodiscard]] constexpr std::string_view diagnosticCodeName(const DiagnosticCode code) noexcept {
    switch (code) {
        case DiagnosticCode::SourceTooLarge: return "VXS001";
        case DiagnosticCode::InvalidUtf8: return "VXS002";
        case DiagnosticCode::TokenLimit: return "VXS003";
        case DiagnosticCode::IdentifierTooLong: return "VXS004";
        case DiagnosticCode::StringTooLong: return "VXS005";
        case DiagnosticCode::NumberTooLong: return "VXS006";
        case DiagnosticCode::UnexpectedCharacter: return "VXS007";
        case DiagnosticCode::UnterminatedString: return "VXS008";
        case DiagnosticCode::InvalidEscape: return "VXS009";
        case DiagnosticCode::InvalidNumber: return "VXS010";
        case DiagnosticCode::ExpectedToken: return "VXS011";
        case DiagnosticCode::ExpectedIdentifier: return "VXS012";
        case DiagnosticCode::ExpectedLiteral: return "VXS013";
        case DiagnosticCode::LimitExceeded: return "VXS014";
        case DiagnosticCode::DuplicateArgument: return "VXS015";
        case DiagnosticCode::UnknownCommand: return "VXS016";
        case DiagnosticCode::UnknownArgument: return "VXS017";
        case DiagnosticCode::MissingArgument: return "VXS018";
        case DiagnosticCode::TypeMismatch: return "VXS019";
        case DiagnosticCode::InvalidComparison: return "VXS020";
        case DiagnosticCode::InvalidEntityRef: return "VXS021";
        case DiagnosticCode::SchemaVersionMismatch: return "VXS022";
        case DiagnosticCode::InvalidSchema: return "VXS023";
        case DiagnosticCode::InvalidPlan: return "VXS024";
    }
    return "VXS000";
}

struct Diagnostic final {
    DiagnosticCode code = DiagnosticCode::ExpectedToken;
    Span span{};
    std::string message;
};

} // namespace vortex_script
