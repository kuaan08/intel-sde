#include "avx_runtime/block_cache.hpp"
#include "avx_runtime/block_builder.hpp"
#include "avx_runtime/executable_memory.hpp"
#include "avx_runtime/feature_policy.hpp"
#include "avx_runtime/memory_address.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    using namespace avxlab;
    const std::uint8_t base_disp8[] = {0x44, 0x48, 0x10}; // [rax+rcx*2+16]
    const MemoryOperand a = decode_memory_operand(base_disp8, sizeof(base_disp8));
    assert(a.has_base && a.base == 0 && a.has_index && a.index == 1 && a.scale == 2 && a.displacement == 16);
    const std::uint8_t rip_disp32[] = {0x05, 0x78, 0x56, 0x34, 0x12};
    const MemoryOperand b = decode_memory_operand(rip_disp32, sizeof(rip_disp32));
    assert(b.rip_relative && b.displacement == 0x12345678 && b.encoded_length == 5);

    IRBlock first; first.start_address = 0x1000; first.instructions.push_back(IRInstruction{});
    BlockCache cache; cache.insert(first, {0x90, 0x90, 0x90});
    assert(cache.find(0x1000) != nullptr && cache.size() == 1);
    cache.invalidate_range(0x1002, 1);
    assert(cache.find(0x1000) == nullptr && cache.size() == 0);

    Coverage partial; const GuestFeatures hidden = derive_guest_features(partial);
    assert(!hidden.avx && !hidden.avx2 && !hidden.osxsave);
    partial.avx1_memory = true; partial.avx2 = true;
    const GuestFeatures complete = derive_guest_features(partial);
    assert(complete.avx && complete.avx2 && complete.osxsave);

    BasicBlockBuilder builder(0x2000);
    IRInstruction first_ir; first_ir.guest_address = 0x2000; first_ir.length = 4;
    IRInstruction second_ir; second_ir.guest_address = 0x2004; second_ir.length = 3;
    builder.append(first_ir); builder.append(second_ir); builder.terminate(BlockTerminator::DirectBranch);
    assert(builder.block().instructions.size() == 2 && builder.block().terminator == BlockTerminator::DirectBranch);

    ExecutableMemoryMap executable;
    executable.register_region(0x4000, 0x1000);
    assert(executable.find(0x4100) != nullptr);
    const std::uint64_t generation = executable.find(0x4100)->generation;
    executable.invalidate_write(0x47f0, 0x40);
    assert(executable.find(0x4100)->generation == generation + 1);
    executable.unregister_region(0x4000);
    assert(executable.find(0x4100) == nullptr);
    std::cout << "PASS: memory addressing, IR blocks, executable-memory invalidation, cache invalidation, and feature gating.\n";
}
