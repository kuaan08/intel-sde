#pragma once

#include "avx_runtime/ir.hpp"
#include <cstdint>
#include <vector>

namespace avxlab {

class BasicBlockBuilder {
public:
    explicit BasicBlockBuilder(std::uint64_t start_address = 0) { block_.start_address = start_address; }
    void append(IRInstruction instruction);
    void terminate(BlockTerminator terminator);
    const IRBlock& block() const { return block_; }
    IRBlock finish() const { return block_; }

private:
    IRBlock block_;
};

} // namespace avxlab
