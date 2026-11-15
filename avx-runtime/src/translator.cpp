#include "avx_runtime/translator.hpp"

#include <algorithm>
#include <cstring>
#include <exception>

namespace avxlab {
namespace {

bool is_ret(std::uint8_t opcode) {
    return opcode == 0xC2 || opcode == 0xC3 || opcode == 0xCA || opcode == 0xCB;
}

bool is_call_rel(std::uint8_t opcode) { return opcode == 0xE8; }
bool is_jump_rel(std::uint8_t opcode) { return opcode == 0xE9 || opcode == 0xEB; }
bool is_conditional_short(std::uint8_t opcode) { return opcode >= 0x70 && opcode <= 0x7F; }
bool is_two_byte_control(std::uint8_t first, std::uint8_t second) {
    if (first != 0x0F) return false;
    return (second >= 0x80 && second <= 0x8F) || second == 0x05 || second == 0x34;
}

std::int64_t signed_value(const std::uint8_t* bytes, std::size_t size) {
    if (size == 1) return static_cast<std::int8_t>(*bytes);
    std::int32_t value = 0;
    std::memcpy(&value, bytes, sizeof(value));
    return value;
}

bool classify_control_flow(const std::vector<std::uint8_t>& bytes, std::size_t offset,
                           std::uint64_t address, IRBlock& block, std::size_t& length) {
    const std::uint8_t opcode = bytes[offset];
    if (is_ret(opcode)) {
        length = opcode == 0xC2 || opcode == 0xCA ? 3 : 1;
        block.terminator = BlockTerminator::Return;
        block.terminator_length = static_cast<std::uint8_t>(length);
        return true;
    }
    if (is_call_rel(opcode) || is_jump_rel(opcode)) {
        length = opcode == 0xEB ? 2 : 5;
        if (offset + length > bytes.size()) return false;
        const std::size_t displacement_size = opcode == 0xEB ? 1 : 4;
        block.direct_target = address + length + signed_value(bytes.data() + offset + 1, displacement_size);
        block.terminator = is_call_rel(opcode) ? BlockTerminator::Call : BlockTerminator::DirectBranch;
        block.terminator_length = static_cast<std::uint8_t>(length);
        return true;
    }
    if (is_conditional_short(opcode)) {
        length = 2;
        if (offset + length > bytes.size()) return false;
        block.direct_target = address + length + signed_value(bytes.data() + offset + 1, 1);
        block.terminator = BlockTerminator::DirectBranch;
        block.terminator_length = 2;
        return true;
    }
    if (opcode == 0x0F && offset + 1 < bytes.size() && is_two_byte_control(opcode, bytes[offset + 1])) {
        length = 2;
        block.terminator = bytes[offset + 1] == 0x05 || bytes[offset + 1] == 0x34
                               ? BlockTerminator::Trap : BlockTerminator::DirectBranch;
        block.terminator_length = 2;
        return true;
    }
    if (opcode == 0xFF) {
        length = 2;
        if (offset + length > bytes.size()) return false;
        const unsigned reg = (bytes[offset + 1] >> 3) & 7U;
        block.terminator = reg == 2 || reg == 3 ? BlockTerminator::Call
                         : reg == 4 || reg == 5 ? BlockTerminator::IndirectBranch
                                                 : BlockTerminator::Trap;
        block.terminator_length = 2;
        return true;
    }
    return false;
}

} // namespace

BlockTranslationResult translate_basic_block(const std::vector<std::uint8_t>& bytes,
                                             std::uint64_t guest_start,
                                             std::size_t max_instructions,
                                             std::size_t max_bytes) {
    BlockTranslationResult result;
    result.block.start_address = guest_start;
    std::size_t offset = 0;
    std::size_t count = 0;
    while (offset < bytes.size() && offset < max_bytes && count < max_instructions) {
        const std::uint64_t address = guest_start + offset;
        std::size_t control_length = 0;
        if (classify_control_flow(bytes, offset, address, result.block, control_length)) {
            if (offset + control_length > bytes.size()) {
                result.stopped_on_unsupported = true;
                result.block.terminator = BlockTerminator::Trap;
                break;
            }
            offset += control_length;
            result.stopped_on_control_flow = true;
            break;
        }

        Decoded decoded;
        std::size_t instruction_length = 0;
        const std::size_t remaining = std::min<std::size_t>(15, bytes.size() - offset);
        for (std::size_t candidate = 3; candidate <= remaining; ++candidate) {
            std::vector<std::uint8_t> slice(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                                            bytes.begin() + static_cast<std::ptrdiff_t>(offset + candidate));
            try {
                decoded = decode(slice);
                instruction_length = candidate;
                break;
            } catch (const std::exception&) {
                // A longer candidate may contain the complete displacement/SIB form.
            }
        }
        if (instruction_length == 0) {
            result.stopped_on_unsupported = true;
            result.block.terminator = BlockTerminator::Trap;
            break;
        }
        decoded.guest_address = address;
        result.block.instructions.push_back(make_ir_instruction(decoded, address,
                                                                static_cast<std::uint8_t>(instruction_length)));
        offset += instruction_length;
        ++count;
    }
    result.source_length = offset;
    result.block.end_address = guest_start + offset;
    if (result.block.instructions.empty() && result.block.terminator == BlockTerminator::Fallthrough)
        result.block.terminator = BlockTerminator::Trap;
    return result;
}

} // namespace avxlab
