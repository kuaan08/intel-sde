#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace avxlab {

struct MemoryOperand {
    bool rip_relative = false;
    bool has_base = false;
    bool has_index = false;
    std::uint8_t base = 0;
    std::uint8_t index = 0;
    std::uint8_t scale = 1;
    std::int64_t displacement = 0;
    std::size_t encoded_length = 0;
};

// Decode a 64-bit ModRM/SIB memory operand. The input starts at ModRM and
// includes all displacement bytes. VEX B'/X' extensions are passed inverted
// as decoded architectural extension bits (0 or 1).
MemoryOperand decode_memory_operand(const std::uint8_t* bytes, std::size_t size,
                                    std::uint8_t vex_b_ext = 0,
                                    std::uint8_t vex_x_ext = 0);
std::string format_memory_operand(const MemoryOperand& operand);

} // namespace avxlab
