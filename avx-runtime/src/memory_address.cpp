#include "avx_runtime/memory_address.hpp"

#include <cstring>
#include <stdexcept>
#include <sstream>

namespace avxlab {
namespace {
void need(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
std::int32_t read_i32(const std::uint8_t* p) { std::int32_t value; std::memcpy(&value, p, sizeof(value)); return value; }
}

MemoryOperand decode_memory_operand(const std::uint8_t* bytes, std::size_t size,
                                    std::uint8_t vex_b_ext, std::uint8_t vex_x_ext) {
    need(bytes != nullptr && size >= 1, "missing ModRM byte");
    const std::uint8_t modrm = bytes[0];
    const unsigned mod = modrm >> 6;
    const unsigned rm = modrm & 7U;
    need(mod != 3, "ModRM selects a register, not memory");
    MemoryOperand result;
    std::size_t offset = 1;
    const bool has_sib = rm == 4;
    unsigned base_field = rm;
    if (has_sib) {
        need(size >= offset + 1, "missing SIB byte");
        const std::uint8_t sib = bytes[offset++];
        const unsigned scale_field = sib >> 6;
        const unsigned index_field = (sib >> 3) & 7U;
        base_field = sib & 7U;
        result.scale = static_cast<std::uint8_t>(1U << scale_field);
        if (index_field != 4) {
            result.has_index = true;
            result.index = static_cast<std::uint8_t>(index_field | (vex_x_ext << 3));
        }
    }
    if (mod == 0 && base_field == 5) {
        result.has_base = false;
        if (has_sib) {
            need(size >= offset + 4, "missing SIB disp32");
            result.displacement = read_i32(bytes + offset); offset += 4;
        } else {
            result.rip_relative = true;
            need(size >= offset + 4, "missing RIP-relative disp32");
            result.displacement = read_i32(bytes + offset); offset += 4;
        }
    } else {
        result.has_base = true;
        result.base = static_cast<std::uint8_t>(base_field | (vex_b_ext << 3));
        if (mod == 1) {
            need(size >= offset + 1, "missing disp8");
            result.displacement = static_cast<std::int8_t>(bytes[offset++]);
        } else if (mod == 2) {
            need(size >= offset + 4, "missing disp32");
            result.displacement = read_i32(bytes + offset); offset += 4;
        }
    }
    result.encoded_length = offset;
    return result;
}

std::string format_memory_operand(const MemoryOperand& operand) {
    std::ostringstream out; out << '[';
    if (operand.rip_relative) out << "rip";
    else if (operand.has_base) out << "r" << static_cast<unsigned>(operand.base);
    else out << "absolute";
    if (operand.has_index) out << "+r" << static_cast<unsigned>(operand.index) << '*' << static_cast<unsigned>(operand.scale);
    if (operand.displacement > 0) out << '+' << operand.displacement;
    else if (operand.displacement < 0) out << operand.displacement;
    out << ']'; return out.str();
}

} // namespace avxlab
