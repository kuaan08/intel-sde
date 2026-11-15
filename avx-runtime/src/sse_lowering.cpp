#include "avx_runtime/sse_lowering.hpp"

#include <algorithm>

namespace avxlab {
namespace {
class Emitter {
public:
    std::vector<std::uint8_t> code;
    void byte(std::uint8_t value) { code.push_back(value); }
    void dword(std::uint32_t value) {
        byte(static_cast<std::uint8_t>(value)); byte(static_cast<std::uint8_t>(value >> 8));
        byte(static_cast<std::uint8_t>(value >> 16)); byte(static_cast<std::uint8_t>(value >> 24));
    }
    void load(unsigned xmm, unsigned offset) {
#if defined(_WIN32)
        constexpr unsigned base = 1; // RCX
#else
        constexpr unsigned base = 7; // RDI
#endif
        byte(0x0F); byte(0x10); byte(static_cast<std::uint8_t>(0x80 | ((xmm & 7U) << 3) | base)); dword(offset);
    }
    void store(unsigned offset, unsigned xmm) {
#if defined(_WIN32)
        constexpr unsigned base = 1;
#else
        constexpr unsigned base = 7;
#endif
        byte(0x0F); byte(0x11); byte(static_cast<std::uint8_t>(0x80 | ((xmm & 7U) << 3) | base)); dword(offset);
    }
    void zero(unsigned xmm) { byte(0x0F); byte(0x57); byte(static_cast<std::uint8_t>(0xC0 | ((xmm & 7U) << 3) | (xmm & 7U))); }
    bool op(IROpcode opcode) {
        std::uint8_t value = 0;
        switch (opcode) {
        case IROpcode::AddPS: value = 0x58; break; case IROpcode::SubPS: value = 0x5C; break;
        case IROpcode::MulPS: value = 0x59; break; case IROpcode::DivPS: value = 0x5E; break;
        case IROpcode::XorPS: value = 0x57; break; case IROpcode::AndPS: value = 0x54; break;
        case IROpcode::OrPS: value = 0x56; break; default: return false;
        }
        byte(0x0F); byte(value); byte(0xC1); // xmm0 := xmm0 op xmm1
        return true;
    }
    bool scalar_op(IROpcode opcode) {
        std::uint8_t value = 0;
        switch (opcode) {
        case IROpcode::AddSS: value = 0x58; break; case IROpcode::SubSS: value = 0x5C; break;
        case IROpcode::MulSS: value = 0x59; break; case IROpcode::DivSS: value = 0x5E; break;
        case IROpcode::MoveSS: value = 0x10; break; default: return false;
        }
        byte(0xF3); byte(0x0F); byte(value); byte(0xC1); // xmm0 := xmm0 op xmm1
        return true;
    }
};

unsigned offset(unsigned reg, unsigned half) { return reg * 32U + half * 16U; }
}

SseLoweringResult lower_block_to_legacy_sse(const IRBlock& block) {
    SseLoweringResult result;
    if (block.instructions.empty()) { result.diagnostic = "cannot lower an empty block"; return result; }
    Emitter emitter;
    for (const IRInstruction& instruction : block.instructions) {
        if (instruction.has_memory_operand) {
            result.diagnostic = "memory operands require the guarded helper path";
            return result;
        }
        if (instruction.vector_bits != 128 && instruction.vector_bits != 256) {
            result.diagnostic = "only 128-bit and 256-bit vector widths are supported";
            return result;
        }
        if (instruction.scalar) {
            const unsigned source = instruction.opcode == IROpcode::MoveSS ? instruction.src1 : instruction.src1;
            emitter.load(0, offset(source, 0));
            emitter.load(1, offset(instruction.opcode == IROpcode::MoveSS ? instruction.src2 : instruction.src2, 0));
            if (!emitter.scalar_op(instruction.opcode)) {
                result.diagnostic = "scalar operation has no legacy SSE lowering";
                return result;
            }
            emitter.store(offset(instruction.dst, 0), 0);
            continue;
        }
        const unsigned halves = instruction.vector_bits == 256 ? 2 : 1;
        if (instruction.opcode == IROpcode::ZeroUpper) {
            emitter.zero(0);
            for (unsigned reg = 0; reg < 16; ++reg) emitter.store(offset(reg, 1), 0);
            continue;
        }
        if (instruction.opcode == IROpcode::Move || instruction.opcode == IROpcode::AddPS ||
            instruction.opcode == IROpcode::SubPS || instruction.opcode == IROpcode::MulPS ||
            instruction.opcode == IROpcode::DivPS || instruction.opcode == IROpcode::XorPS ||
            instruction.opcode == IROpcode::AndPS || instruction.opcode == IROpcode::OrPS) {
            for (unsigned half = 0; half < halves; ++half) {
                const unsigned source = instruction.opcode == IROpcode::Move ? instruction.src2 : instruction.src1;
                emitter.load(0, offset(source, half));
                if (instruction.opcode == IROpcode::Move) {
                    emitter.store(offset(instruction.dst, half), 0);
                } else {
                    emitter.load(1, offset(instruction.src2, half));
                    if (!emitter.op(instruction.opcode)) {
                        result.diagnostic = "operation has no legacy SSE lowering";
                        return result;
                    }
                    emitter.store(offset(instruction.dst, half), 0);
                }
            }
            if (instruction.vector_bits == 128) {
                emitter.zero(0);
                emitter.store(offset(instruction.dst, 1), 0);
            }
        } else {
            result.diagnostic = "unsupported IR opcode";
            return result;
        }
    }
    if (block.terminator == BlockTerminator::Trap || block.terminator == BlockTerminator::IndirectBranch ||
        block.terminator == BlockTerminator::Call || block.terminator == BlockTerminator::DirectBranch ||
        block.terminator == BlockTerminator::Return) {
        result.diagnostic = "control-flow exits require the dispatcher backend";
        return result;
    }
    emitter.byte(0xC3);
    result.supported = true;
    result.code = std::move(emitter.code);
    return result;
}

bool contains_vex_or_evex_prefix(const std::vector<std::uint8_t>& code) {
    return std::find(code.begin(), code.end(), 0xC4) != code.end() ||
           std::find(code.begin(), code.end(), 0xC5) != code.end() ||
           std::find(code.begin(), code.end(), 0x62) != code.end();
}

} // namespace avxlab
