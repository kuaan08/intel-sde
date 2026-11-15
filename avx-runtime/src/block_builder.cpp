#include "avx_runtime/block_builder.hpp"

#include <stdexcept>

namespace avxlab {

void BasicBlockBuilder::append(IRInstruction instruction) {
    if (instruction.length == 0) throw std::invalid_argument("IR instruction length must be non-zero");
    if (!block_.instructions.empty()) {
        const IRInstruction& previous = block_.instructions.back();
        const std::uint64_t expected = previous.guest_address + previous.length;
        if (instruction.guest_address != expected) throw std::invalid_argument("IR instructions must be contiguous");
    } else {
        block_.start_address = instruction.guest_address;
    }
    if (block_.terminator != BlockTerminator::Fallthrough) throw std::logic_error("cannot append after block terminator");
    block_.instructions.push_back(instruction);
}

void BasicBlockBuilder::terminate(BlockTerminator terminator) {
    if (terminator == BlockTerminator::Fallthrough) throw std::invalid_argument("explicit terminator cannot be Fallthrough");
    if (block_.instructions.empty()) throw std::logic_error("cannot terminate an empty block");
    block_.terminator = terminator;
}

} // namespace avxlab
