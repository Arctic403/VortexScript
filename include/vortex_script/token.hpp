#pragma once

#include "vortex_script/diagnostic.hpp"

#include <string>

namespace vortex_script {

enum class TokenKind : unsigned char {
    Identifier,
    Number,
    String,
    LBrace,
    RBrace,
    Colon,
    Equal,
    LessEqual,
    Semicolon,
    Eof,
};

struct Token final {
    TokenKind kind = TokenKind::Eof;
    std::string text;
    Span span{};
};

} // namespace vortex_script
