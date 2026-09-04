#include "vortex_script/compiler.hpp"

extern "C" int vortex_script_android_link_smoke() {
    const auto result = vortex_script::compile("transaction T { command noop {} }");
    return result.ok() ? 1 : 0;
}
