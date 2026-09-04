#pragma once

#include "vortex_script/ast.hpp"
#include "vortex_script/diagnostic.hpp"
#include "vortex_script/token.hpp"

#include <string_view>
#include <vector>

namespace vortex_script::detail {
[[nodiscard]] bool lex(std::string_view source, std::vector<Token>& tokens, Diagnostic& diagnostic);
[[nodiscard]] bool parse(const std::vector<Token>& tokens, Program& program, Diagnostic& diagnostic);
} // namespace vortex_script::detail
