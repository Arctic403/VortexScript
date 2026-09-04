#pragma once

#include "vortex_script/diagnostic.hpp"
#include "vortex_script/engine_contract.hpp"
#include "vortex_script/plan.hpp"

#include <string_view>
#include <vector>

namespace vortex_script {

struct CompileResult final {
    Plan plan;
    std::vector<Diagnostic> diagnostics;

    [[nodiscard]] bool ok() const noexcept { return diagnostics.empty(); }
};

[[nodiscard]] CompileResult compile(std::string_view source, const CommandSchema* schema = nullptr);

} // namespace vortex_script
