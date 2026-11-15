#include "avx_runtime/decoder.hpp"
#include "avx_runtime/ir.hpp"

#include <cctype>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace avxlab {
namespace {
void fail(const std::string& text) { throw std::runtime_error(text); }
}

Decoded decode(const std::vector<std::uint8_t>& b) {
    if (b.size() < 3) fail("truncated VEX instruction");
    std::size_t off = 0; unsigned map = 0, pp = 0, w = 0, l = 0, v = 0, r = 0, bx = 0, x = 0;
    if (b[0] == 0xC5) {
        std::uint8_t p = b[1]; map = 1; r = ((p >> 7) & 1U) ^ 1U; v = (p >> 3) & 15U; l = (p >> 2) & 1U; pp = p & 3U; off = 2;
    } else if (b[0] == 0xC4) {
        if (b.size() < 4) fail("truncated VEX3 instruction");
        std::uint8_t p0 = b[1], p1 = b[2]; map = p0 & 31U; r = ((p0 >> 7) & 1U) ^ 1U; x = ((p0 >> 6) & 1U) ^ 1U; bx = ((p0 >> 5) & 1U) ^ 1U; w = (p1 >> 7) & 1U; v = (p1 >> 3) & 15U; l = (p1 >> 2) & 1U; pp = p1 & 3U; off = 3;
    } else fail("not a VEX instruction");
    if (map != 1 || pp != 0 || w != 0 || off >= b.size()) fail("unsupported VEX form");
    if (b[off] == 0x77) {
        if (l || v != 15 || b.size() != off + 1) fail("invalid VZEROUPPER");
        return {Op::ZeroUpper, 0, 0, 0, 256};
    }
    if (off + 1 >= b.size()) fail("missing ModRM byte");
    const std::uint8_t modrm = b[off + 1];
    const unsigned mod = modrm >> 6;
    const unsigned reg = ((modrm >> 3) & 7U) | (r << 3);
    const unsigned rm = (modrm & 7U) | (bx << 3);
    const unsigned src1 = (~v) & 15U;
    const unsigned bits = l ? 256 : 128;
    Op op;
    switch (b[off]) {
    case 0x58: op = Op::AddPS; break; case 0x5C: op = Op::SubPS; break; case 0x59: op = Op::MulPS; break; case 0x5E: op = Op::DivPS; break;
    case 0x57: op = Op::XorPS; break; case 0x54: op = Op::AndPS; break; case 0x56: op = Op::OrPS; break;
    case 0x10: case 0x28: case 0x11: case 0x29: if (v != 15) fail("invalid VMOV reserved vvvv"); op = Op::Move; break;
    default: fail("unsupported AVX1 opcode");
    }
    if (mod == 3) {
        if (x != 0 || b.size() != off + 2) fail("invalid register-only VEX encoding");
        if (op == Op::Move && (b[off] == 0x11 || b[off] == 0x29)) return {op, rm, 0, reg, bits};
        if (op == Op::Move) return {op, reg, 0, rm, bits};
        return {op, reg, src1, rm, bits};
    }
    const MemoryOperand memory = decode_memory_operand(b.data() + off + 1, b.size() - off - 1,
                                                        static_cast<std::uint8_t>(bx), static_cast<std::uint8_t>(x));
    if (b.size() != off + 1 + memory.encoded_length) fail("trailing bytes after VEX memory instruction");
    if (op == Op::Move && (b[off] == 0x11 || b[off] == 0x29)) return {op, 0, 0, reg, bits, true, true, memory};
    return {op, reg, src1, 0, bits, true, false, memory};
}

std::vector<std::uint8_t> parse_hex(const std::string& text) {
    std::string compact;
    for (char c : text) { if (std::isspace(static_cast<unsigned char>(c)) || c == ':') continue; if (!std::isxdigit(static_cast<unsigned char>(c))) fail("invalid hexadecimal character in input"); compact += c; }
    if (compact.size() % 2) fail("hex input must have an even number of digits");
    std::vector<std::uint8_t> out; for (std::size_t i = 0; i < compact.size(); i += 2) out.push_back(static_cast<std::uint8_t>(std::stoul(compact.substr(i, 2), nullptr, 16))); return out;
}

namespace {
IROpcode to_ir_opcode(Op op) {
    switch (op) { case Op::AddPS: return IROpcode::AddPS; case Op::SubPS: return IROpcode::SubPS; case Op::MulPS: return IROpcode::MulPS; case Op::DivPS: return IROpcode::DivPS; case Op::XorPS: return IROpcode::XorPS; case Op::AndPS: return IROpcode::AndPS; case Op::OrPS: return IROpcode::OrPS; case Op::Move: return IROpcode::Move; case Op::ZeroUpper: return IROpcode::ZeroUpper; default: return IROpcode::Unsupported; }
}
}

IRInstruction make_ir_instruction(const Decoded& decoded, std::uint64_t guest_address, std::uint8_t length) {
    IRInstruction instruction;
    instruction.opcode = to_ir_opcode(decoded.op);
    instruction.dst = static_cast<std::uint8_t>(decoded.dst);
    instruction.src1 = static_cast<std::uint8_t>(decoded.src1);
    instruction.src2 = static_cast<std::uint8_t>(decoded.src2);
    instruction.vector_bits = static_cast<std::uint16_t>(decoded.bits);
    instruction.guest_address = guest_address;
    instruction.length = length;
    instruction.has_memory_operand = decoded.memory;
    instruction.memory = decoded.memory_operand;
    instruction.memory_width_bytes = static_cast<std::uint16_t>(decoded.bits / 8);
    instruction.exception_behavior = decoded.memory ? ExceptionBehavior::MayFaultOnMemory
                                                    : ExceptionBehavior::MayRaiseFloatingPoint;
    if (decoded.op == Op::ZeroUpper) {
        instruction.access.ymm_write_mask = 0xFFFF;
    } else if (decoded.memory && decoded.memory_destination) {
        instruction.memory_access = MemoryAccess::Write;
        instruction.access.ymm_read_mask = static_cast<std::uint16_t>(1U << decoded.src2);
    } else {
        instruction.memory_access = decoded.memory ? MemoryAccess::Read : MemoryAccess::None;
        instruction.access.ymm_read_mask = decoded.op == Op::Move
            ? 0 : static_cast<std::uint16_t>(1U << decoded.src1);
        if (decoded.op != Op::Move)
            instruction.access.ymm_read_mask |= static_cast<std::uint16_t>(1U << decoded.src2);
        instruction.access.ymm_write_mask = static_cast<std::uint16_t>(1U << decoded.dst);
    }
    if (!decoded.memory && decoded.op == Op::Move) {
        instruction.access.ymm_read_mask = static_cast<std::uint16_t>(1U << decoded.src2);
        instruction.access.ymm_write_mask = static_cast<std::uint16_t>(1U << decoded.dst);
    }
    return instruction;
}

} // namespace avxlab
