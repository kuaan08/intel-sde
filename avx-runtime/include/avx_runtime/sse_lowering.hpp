#pragma once

#include "avx_runtime/ir.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace avxlab {

struct SseLoweringResult {
    bool supported = false;
    std::vector<std::uint8_t> code;
    std::string diagnostic;
};

// Emits a callable SysV/Windows-x64 function taking a pointer to 16 logical
// YMM registers laid out as 32 bytes each. The current native backend accepts
// register-register packed-single IR; memory forms deliberately fail closed.
SseLoweringResult lower_block_to_legacy_sse(const IRBlock& block);

bool contains_vex_or_evex_prefix(const std::vector<std::uint8_t>& code);

} // namespace avxlab
