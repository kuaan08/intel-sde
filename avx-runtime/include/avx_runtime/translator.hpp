#pragma once

#include "avx_runtime/decoder.hpp"
#include "avx_runtime/ir.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace avxlab {

IRInstruction make_ir_instruction(const Decoded& decoded, std::uint64_t guest_address,
                                  std::uint8_t length);

struct BlockTranslationResult {
    IRBlock block;
    std::size_t source_length = 0;
    bool stopped_on_control_flow = false;
    bool stopped_on_unsupported = false;
};

// Translate a bounded byte span into a safe linear IR block. Unsupported bytes
// and control-flow instructions terminate the block; they are never executed.
BlockTranslationResult translate_basic_block(const std::vector<std::uint8_t>& bytes,
                                             std::uint64_t guest_start,
                                             std::size_t max_instructions = 64,
                                             std::size_t max_bytes = 4096);

} // namespace avxlab
