#include "vortex_script/compiler.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {
[[nodiscard]] std::string readFile(const char* path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error(std::string("could not open ") + path);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}
}

int main(int argc, char** argv) {
    if (argc != 3 || std::string_view(argv[1]) != "check") {
        std::cerr << "usage: vortex-script check <file.vxs>\n";
        return 2;
    }
    try {
        const std::string source = readFile(argv[2]);
        const auto result = vortex_script::compile(source);
        if (!result.ok()) {
            for (const auto& diagnostic : result.diagnostics) {
                std::cerr << vortex_script::diagnosticCodeName(diagnostic.code) << " bytes "
                          << diagnostic.span.start << ".." << diagnostic.span.end << ": "
                          << diagnostic.message << '\n';
            }
            return 1;
        }
        std::size_t commands = 0;
        for (const auto& transaction : result.plan.transactions) commands += transaction.commands.size();
        std::cout << "ok: " << result.plan.transactions.size() << " transaction(s), "
                  << commands << " command(s), " << result.plan.strings.size() << " interned string(s)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "vortex-script: " << error.what() << '\n';
        return 1;
    }
}
