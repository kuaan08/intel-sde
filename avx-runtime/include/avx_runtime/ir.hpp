#pragma once

#include "avx_runtime/memory_address.hpp"
#include <cstdint>
#include <vector>

namespace avxlab {

enum class IROpcode { AddPS, SubPS, MulPS, DivPS, XorPS, AndPS, OrPS, Move, ZeroUpper, Unsupported };

enum class BlockTerminator { Fallthrough, DirectBranch, IndirectBranch, Call, Return, Trap };

struct IRInstruction {
    IROpcode opcode = IROpcode::Unsupported;
    std::uint8_t dst = 0;
    std::uint8_t src1 = 0;
    std::uint8_t src2 = 0;
    std::uint16_t vector_bits = 128;
    std::uint64_t guest_address = 0;
    std::uint8_t length = 0;
    bool has_memory_operand = false;
    MemoryOperand memory;
};

struct IRBlock {
    std::uint64_t start_address = 0;
    std::vector<IRInstruction> instructions;
    BlockTerminator terminator = BlockTerminator::Fallthrough;
    std::uint64_t generation = 0;
};

} // namespace avxlab
