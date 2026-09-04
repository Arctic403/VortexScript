#pragma once

#include <cstddef>

namespace vortex_script::limits {
inline constexpr std::size_t kMaxSourceBytes = 1024U * 1024U;
inline constexpr std::size_t kMaxTokens = 65536U;
inline constexpr std::size_t kMaxIdentifierBytes = 128U;
inline constexpr std::size_t kMaxStringBytes = 1024U * 1024U;
inline constexpr std::size_t kMaxNumberBytes = 128U;
inline constexpr std::size_t kMaxTransactions = 1024U;
inline constexpr std::size_t kMaxCommands = 4096U;
inline constexpr std::size_t kMaxArgumentsPerCommand = 128U;
inline constexpr std::size_t kMaxArgumentsTotal = 32768U;
inline constexpr std::size_t kMaxInternedStrings = 16384U;
inline constexpr std::size_t kMaxInternedStringBytes = 2U * 1024U * 1024U;
} // namespace vortex_script::limits
