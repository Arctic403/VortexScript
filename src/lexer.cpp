#include "internal.hpp"

#include "vortex_script/limits.hpp"

#include <cctype>
#include <cstdint>
#include <string>

namespace vortex_script::detail {
namespace {

[[nodiscard]] bool isIdentStart(const unsigned char c) noexcept {
    return std::isalpha(c) != 0 || c == '_';
}

[[nodiscard]] bool isIdentContinue(const unsigned char c) noexcept {
    return isIdentStart(c) || std::isdigit(c) != 0;
}

[[nodiscard]] bool fail(Diagnostic& diagnostic, const DiagnosticCode code,
                        const std::size_t start, const std::size_t end, std::string message) {
    diagnostic = Diagnostic{code,
                            Span{static_cast<std::uint32_t>(start), static_cast<std::uint32_t>(end)},
                            std::move(message)};
    return false;
}

[[nodiscard]] bool validateUtf8(const std::string_view source, std::size_t& badOffset) noexcept {
    const auto* bytes = reinterpret_cast<const unsigned char*>(source.data());
    std::size_t i = 0;
    while (i < source.size()) {
        const unsigned char c = bytes[i];
        if (c <= 0x7F) { ++i; continue; }
        std::size_t need = 0;
        std::uint32_t codepoint = 0;
        std::uint32_t minimum = 0;
        if ((c & 0xE0U) == 0xC0U) { need = 1; codepoint = c & 0x1FU; minimum = 0x80U; }
        else if ((c & 0xF0U) == 0xE0U) { need = 2; codepoint = c & 0x0FU; minimum = 0x800U; }
        else if ((c & 0xF8U) == 0xF0U) { need = 3; codepoint = c & 0x07U; minimum = 0x10000U; }
        else { badOffset = i; return false; }
        if (i + need >= source.size()) { badOffset = i; return false; }
        for (std::size_t j = 1; j <= need; ++j) {
            const unsigned char next = bytes[i + j];
            if ((next & 0xC0U) != 0x80U) { badOffset = i + j; return false; }
            codepoint = (codepoint << 6U) | (next & 0x3FU);
        }
        if (codepoint < minimum || codepoint > 0x10FFFFU ||
            (codepoint >= 0xD800U && codepoint <= 0xDFFFU)) {
            badOffset = i;
            return false;
        }
        i += need + 1;
    }
    return true;
}

[[nodiscard]] bool pushToken(std::vector<Token>& tokens, Diagnostic& diagnostic,
                             TokenKind kind, std::string text, std::size_t start, std::size_t end) {
    if (tokens.size() >= limits::kMaxTokens) {
        return fail(diagnostic, DiagnosticCode::TokenLimit, start, end, "token limit exceeded");
    }
    tokens.push_back(Token{kind, std::move(text),
                           Span{static_cast<std::uint32_t>(start), static_cast<std::uint32_t>(end)}});
    return true;
}

} // namespace

bool lex(const std::string_view source, std::vector<Token>& tokens, Diagnostic& diagnostic) {
    tokens.clear();
    if (source.size() > limits::kMaxSourceBytes) {
        return fail(diagnostic, DiagnosticCode::SourceTooLarge, 0, source.size(), "source exceeds 1 MiB limit");
    }
    std::size_t badOffset = 0;
    if (!validateUtf8(source, badOffset)) {
        return fail(diagnostic, DiagnosticCode::InvalidUtf8, badOffset, badOffset + 1, "source is not valid UTF-8");
    }

    std::size_t i = 0;
    while (i < source.size()) {
        const unsigned char c = static_cast<unsigned char>(source[i]);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { ++i; continue; }
        if (c == '/' && i + 1 < source.size() && source[i + 1] == '/') {
            i += 2;
            while (i < source.size() && source[i] != '\n') ++i;
            continue;
        }
        if (c == '{' || c == '}' || c == ':' || c == '=' || c == ';') {
            TokenKind kind = TokenKind::LBrace;
            if (c == '}') kind = TokenKind::RBrace;
            else if (c == ':') kind = TokenKind::Colon;
            else if (c == '=') kind = TokenKind::Equal;
            else if (c == ';') kind = TokenKind::Semicolon;
            if (!pushToken(tokens, diagnostic, kind, {}, i, i + 1)) return false;
            ++i;
            continue;
        }
        if (c == '<') {
            if (i + 1 < source.size() && source[i + 1] == '=') {
                if (!pushToken(tokens, diagnostic, TokenKind::LessEqual, {}, i, i + 2)) return false;
                i += 2;
                continue;
            }
            return fail(diagnostic, DiagnosticCode::UnexpectedCharacter, i, i + 1, "expected '<='");
        }
        if (c == '"') {
            const std::size_t start = i++;
            std::string value;
            while (i < source.size() && source[i] != '"') {
                if (source[i] == '\\') {
                    const std::size_t escapeStart = i++;
                    if (i >= source.size()) {
                        return fail(diagnostic, DiagnosticCode::UnterminatedString, start, source.size(), "unterminated string literal");
                    }
                    switch (source[i]) {
                        case 'n': value.push_back('\n'); break;
                        case 't': value.push_back('\t'); break;
                        case 'r': value.push_back('\r'); break;
                        case '"': value.push_back('"'); break;
                        case '\\': value.push_back('\\'); break;
                        default:
                            return fail(diagnostic, DiagnosticCode::InvalidEscape, escapeStart, i + 1,
                                        "unsupported string escape");
                    }
                    ++i;
                } else {
                    const unsigned char byte = static_cast<unsigned char>(source[i]);
                    const std::size_t charBytes = byte < 0x80U ? 1U : ((byte & 0xE0U) == 0xC0U ? 2U : ((byte & 0xF0U) == 0xE0U ? 3U : 4U));
                    value.append(source.substr(i, charBytes));
                    i += charBytes;
                }
                if (value.size() > limits::kMaxStringBytes) {
                    return fail(diagnostic, DiagnosticCode::StringTooLong, start, i, "string literal exceeds limit");
                }
            }
            if (i >= source.size()) {
                return fail(diagnostic, DiagnosticCode::UnterminatedString, start, source.size(), "unterminated string literal");
            }
            ++i;
            if (!pushToken(tokens, diagnostic, TokenKind::String, std::move(value), start, i)) return false;
            continue;
        }
        if (isIdentStart(c)) {
            const std::size_t start = i++;
            while (i < source.size() && isIdentContinue(static_cast<unsigned char>(source[i]))) ++i;
            if (i - start > limits::kMaxIdentifierBytes) {
                return fail(diagnostic, DiagnosticCode::IdentifierTooLong, start, i, "identifier exceeds 128-byte limit");
            }
            if (!pushToken(tokens, diagnostic, TokenKind::Identifier, std::string(source.substr(start, i - start)), start, i)) return false;
            continue;
        }
        if (c == '-' || std::isdigit(c) != 0) {
            const std::size_t start = i;
            if (source[i] == '-') {
                ++i;
                if (i >= source.size() || std::isdigit(static_cast<unsigned char>(source[i])) == 0) {
                    return fail(diagnostic, DiagnosticCode::InvalidNumber, start, i, "'-' must be followed by a digit");
                }
            }
            while (i < source.size() && std::isdigit(static_cast<unsigned char>(source[i])) != 0) ++i;
            if (i < source.size() && source[i] == '.') {
                ++i;
                if (i >= source.size() || std::isdigit(static_cast<unsigned char>(source[i])) == 0) {
                    return fail(diagnostic, DiagnosticCode::InvalidNumber, start, i, "fraction requires digits after decimal point");
                }
                while (i < source.size() && std::isdigit(static_cast<unsigned char>(source[i])) != 0) ++i;
            }
            if (i < source.size() && (source[i] == 'e' || source[i] == 'E')) {
                ++i;
                if (i < source.size() && (source[i] == '+' || source[i] == '-')) ++i;
                if (i >= source.size() || std::isdigit(static_cast<unsigned char>(source[i])) == 0) {
                    return fail(diagnostic, DiagnosticCode::InvalidNumber, start, i, "exponent requires digits");
                }
                while (i < source.size() && std::isdigit(static_cast<unsigned char>(source[i])) != 0) ++i;
            }
            if (i - start > limits::kMaxNumberBytes) {
                return fail(diagnostic, DiagnosticCode::NumberTooLong, start, i, "number literal exceeds 128-byte limit");
            }
            if (!pushToken(tokens, diagnostic, TokenKind::Number, std::string(source.substr(start, i - start)), start, i)) return false;
            continue;
        }

        return fail(diagnostic, DiagnosticCode::UnexpectedCharacter, i, i + 1, "unexpected character");
    }

    return pushToken(tokens, diagnostic, TokenKind::Eof, {}, source.size(), source.size());
}

} // namespace vortex_script::detail
