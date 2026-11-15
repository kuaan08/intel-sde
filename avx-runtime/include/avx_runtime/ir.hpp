#pragma once

#include "avx_runtime/memory_address.hpp"
#include <cstdint>
#include <vector>

namespace avxlab {

enum class IROpcode { AddPS, SubPS, MulPS, DivPS, XorPS, AndPS, OrPS, Move, ZeroUpper, Unsupported };

enum class BlockTerminator { Fallthrough, DirectBranch, IndirectBranch, Call, Return, Trap };

enum class MemoryAccess { None, Read, Write };
enum class AlignmentRequirement { Any, NaturalVector, Exact16, Exact32 };
enum class ExceptionBehavior { None, MayFaultOnMemory, MayRaiseFloatingPoint };

struct RegisterAccess {
    std::uint16_t gpr_read_mask = 0;
    std::uint16_t gpr_write_mask = 0;
    std::uint16_t ymm_read_mask = 0;
    std::uint16_t ymm_write_mask = 0;
    bool reads_mxcsr = false;
    bool writes_mxcsr = false;
};

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
    MemoryAccess memory_access = MemoryAccess::None;
    std::uint16_t memory_width_bytes = 0;
    AlignmentRequirement alignment = AlignmentRequirement::Any;
    ExceptionBehavior exception_behavior = ExceptionBehavior::None;
    RegisterAccess access;
};

struct IRBlock {
    std::uint64_t start_address = 0;
    std::vector<IRInstruction> instructions;
    BlockTerminator terminator = BlockTerminator::Fallthrough;
    std::uint64_t end_address = 0;
    std::uint64_t direct_target = 0;
    std::uint8_t terminator_length = 0;
    std::uint64_t generation = 0;
};

} // namespace avxlab
