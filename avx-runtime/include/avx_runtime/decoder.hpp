#pragma once

#include "avx_runtime/memory_address.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace avxlab {

enum class Op { AddPS, SubPS, MulPS, DivPS, XorPS, AndPS, OrPS, Move, ZeroUpper };

struct Decoded {
    Op op = Op::AddPS;
    unsigned dst = 0;
    unsigned src1 = 0;
    unsigned src2 = 0;
    unsigned bits = 128;
    bool memory = false;
    bool memory_destination = false;
    MemoryOperand memory_operand{};
    std::uint64_t guest_address = 0;
};

Decoded decode(const std::vector<std::uint8_t>& bytes);
std::vector<std::uint8_t> parse_hex(const std::string& text);

} // namespace avxlab
