#include "avx_runtime/block_cache.hpp"
#include "avx_runtime/block_builder.hpp"
#include "avx_runtime/decoder.hpp"
#include "avx_runtime/dispatcher.hpp"
#include "avx_runtime/executable_memory.hpp"
#include "avx_runtime/feature_policy.hpp"
#include "avx_runtime/memory_address.hpp"
#include "avx_runtime/process_runtime.hpp"
#include "avx_runtime/sse_lowering.hpp"
#include "avx_runtime/translator.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace {

void* make_executable_copy(const std::vector<std::uint8_t>& code) {
#ifdef _WIN32
    void* memory = VirtualAlloc(nullptr, code.size(), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!memory) return nullptr;
    std::memcpy(memory, code.data(), code.size());
    DWORD old_protection = 0;
    if (!VirtualProtect(memory, code.size(), PAGE_EXECUTE_READ, &old_protection)) {
        VirtualFree(memory, 0, MEM_RELEASE);
        return nullptr;
    }
    FlushInstructionCache(GetCurrentProcess(), memory, code.size());
    return memory;
#else
    const long page_size = sysconf(_SC_PAGESIZE);
    const std::size_t allocation = ((code.size() + static_cast<std::size_t>(page_size) - 1) /
                                    static_cast<std::size_t>(page_size)) * static_cast<std::size_t>(page_size);
    void* memory = mmap(nullptr, allocation, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) return nullptr;
    std::memcpy(memory, code.data(), code.size());
    if (mprotect(memory, allocation, PROT_READ | PROT_EXEC) != 0) {
        munmap(memory, allocation);
        return nullptr;
    }
    return memory;
#endif
}

void release_executable_copy(void* memory, std::size_t code_size) {
#ifdef _WIN32
    (void)code_size;
    VirtualFree(memory, 0, MEM_RELEASE);
#else
    const long page_size = sysconf(_SC_PAGESIZE);
    const std::size_t allocation = ((code_size + static_cast<std::size_t>(page_size) - 1) /
                                    static_cast<std::size_t>(page_size)) * static_cast<std::size_t>(page_size);
    munmap(memory, allocation);
#endif
}

} // namespace

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
    IRBlock variant = first; variant.generation = 2;
    cache.insert(variant, {0x90, 0x90, 0x91}, 42, 7, 3);
    const BlockCacheKey variant_key{42, 0x1000, 0, 7, 3};
    assert(cache.find(variant_key) == nullptr); // source hash is part of the key
    assert(cache.size() == 2);
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

    const auto linear = translate_basic_block({0xC5, 0xF0, 0x58, 0xC2, 0xC5, 0xF8, 0x77, 0xC3}, 0x8000);
    assert(linear.block.instructions.size() == 2);
    assert(linear.block.terminator == BlockTerminator::Return);
    assert(linear.block.end_address == 0x8008 && linear.source_length == 8);

    const auto branch = translate_basic_block({0xC5, 0xF0, 0x58, 0xC2, 0xEB, 0x02, 0x90, 0x90}, 0x9000);
    assert(branch.block.instructions.size() == 1);
    assert(branch.stopped_on_control_flow);
    assert(branch.block.terminator == BlockTerminator::DirectBranch);
    assert(branch.block.direct_target == 0x9008);

    const auto unsupported = translate_basic_block({0xC5, 0xF0, 0xFA, 0xC2}, 0xA000);
    assert(unsupported.block.instructions.empty());
    assert(unsupported.stopped_on_unsupported);
    assert(unsupported.block.terminator == BlockTerminator::Trap);

    const auto lowered = lower_block_to_legacy_sse(linear.block);
    assert(!lowered.supported);
    assert(lowered.diagnostic.find("control-flow") != std::string::npos);
    const auto straight = translate_basic_block({0xC5, 0xF0, 0x58, 0xC2, 0xC5, 0xF0, 0x5C, 0xCA}, 0xB000);
    const auto straight_sse = lower_block_to_legacy_sse(straight.block);
    assert(straight_sse.supported && !straight_sse.code.empty());
    assert(!contains_vex_or_evex_prefix(straight_sse.code));
    assert(straight_sse.code.back() == 0xC3);

    // Execute the generated legacy-SSE block through a W^X code page. The
    // generated function receives the logical YMM state pointer in the native
    // first-argument register (RDI on SysV, RCX on Windows x64).
    const auto executable_code = lower_block_to_legacy_sse(
        translate_basic_block({0xC5, 0xF0, 0x58, 0xC2}, 0xB100).block);
    assert(executable_code.supported);
    std::array<std::uint8_t, 16 * 32> guest_state{};
    float left = 2.0f;
    float right = 3.0f;
    std::memcpy(guest_state.data() + 32, &left, sizeof(left));
    std::memcpy(guest_state.data() + 64, &right, sizeof(right));
    void* executable_code_page = make_executable_copy(executable_code.code);
    assert(executable_code_page != nullptr);
    using LoweredFunction = void (*)(void*);
    reinterpret_cast<LoweredFunction>(executable_code_page)(guest_state.data());
    float lowered_result = 0.0f;
    std::memcpy(&lowered_result, guest_state.data(), sizeof(lowered_result));
    assert(lowered_result == 5.0f);
    release_executable_copy(executable_code_page, executable_code.code.size());

    const auto memory_block = translate_basic_block({0xC5, 0xF0, 0x58, 0x00}, 0xC000);
    const auto memory_sse = lower_block_to_legacy_sse(memory_block.block);
    assert(!memory_sse.supported && memory_sse.diagnostic.find("memory") != std::string::npos);

    const Decoded scalar_decoded = decode(parse_hex("c5 f2 58 c2"));
    assert(scalar_decoded.scalar && scalar_decoded.op == Op::AddSS && scalar_decoded.src1 == 1 && scalar_decoded.src2 == 2);
    const auto scalar_block = translate_basic_block({0xC5, 0xF2, 0x58, 0xC2}, 0xC100);
    const auto scalar_sse = lower_block_to_legacy_sse(scalar_block.block);
    assert(scalar_sse.supported && !scalar_sse.code.empty());
    assert(!contains_vex_or_evex_prefix(scalar_sse.code));
    const Decoded scalar_load = decode(parse_hex("c5 f2 10 00"));
    assert(scalar_load.scalar && scalar_load.memory && scalar_load.memory_operand.has_base);
    const Decoded scalar_store = decode(parse_hex("c5 fa 11 00"));
    assert(scalar_store.scalar && scalar_store.memory && scalar_store.memory_destination && scalar_store.src2 == 0);

    ProcessRuntimeTracker tracker;
    tracker.register_executable_region(0xD000, 0x1000);
    assert(tracker.generation_at(0xD100) == 1);
    tracker.notify_code_write(0xD080, 16);
    assert(tracker.generation_at(0xD100) == 2);
    auto& thread = tracker.threads().get_or_create(7);
    thread.mxcsr = 0x1F80;
    thread.xcr0 = 3;
    assert(tracker.threads().contains(7) && tracker.threads().size() == 1);
    assert(tracker.threads().erase(7) && !tracker.threads().contains(7));
    UnsupportedInstructionReport report; report.thread_id = 9; report.instruction_pointer = 0xD100; report.reason = "unsupported opcode";
    tracker.add_diagnostic(std::move(report));
    assert(tracker.diagnostics().size() == 1);
    assert(!guest_feature_enabled(Coverage{}, "avx"));
    const VirtualCpuidResult hidden_cpuid = virtual_cpuid(Coverage{}, 1);
    assert((hidden_cpuid.ecx & (1U << 28)) == 0);
    std::uint64_t xcr0 = 0;
    assert(!virtual_xgetbv(Coverage{}, 0, &xcr0));
    Coverage exposed; exposed.avx1_memory = true; exposed.avx2 = true;
    const VirtualCpuidResult exposed_cpuid = virtual_cpuid(exposed, 1);
    assert((exposed_cpuid.ecx & (1U << 28)) != 0);
    assert((virtual_cpuid(exposed, 7).ebx & (1U << 5)) != 0);
    assert(virtual_xgetbv(exposed, 0, &xcr0) && xcr0 == 6);

    TranslationDispatcher dispatcher(&tracker);
    const std::vector<std::uint8_t> guest_code = {0xC5, 0xF2, 0x58, 0xC2, 0xC3};
    const GuestCodeReader reader = [&](std::uint64_t address, std::size_t size,
                                       std::vector<std::uint8_t>* output) {
        if (!output || address != 0xD000 || size < guest_code.size()) return false;
        *output = guest_code;
        return true;
    };
    DispatchRequest request; request.module_id = 1; request.thread_id = 11;
    request.guest_address = 0xD000; request.feature_profile = 7; request.module = "fixture";
    const DispatchResult translated_result = dispatcher.dispatch(request, reader);
    assert(translated_result.status == DispatchStatus::Translated && dispatcher.cache_size() == 1);
    const DispatchResult cached_result = dispatcher.dispatch(request, reader);
    assert(cached_result.status == DispatchStatus::CacheHit && cached_result.block.start_address == 0xD000);
    dispatcher.invalidate_code(0xD000, guest_code.size());
    const DispatchResult retransformed = dispatcher.dispatch(request, reader);
    assert(retransformed.status == DispatchStatus::Translated && dispatcher.cache_size() == 1);

    const GuestCodeReader unsupported_reader = [](std::uint64_t, std::size_t,
                                                   std::vector<std::uint8_t>* output) {
        *output = {0xC5, 0xF2, 0xFA, 0xC2};
        return true;
    };
    request.guest_address = 0xD100;
    const DispatchResult unsupported_result = dispatcher.dispatch(request, unsupported_reader);
    assert(unsupported_result.status == DispatchStatus::Unsupported && !tracker.diagnostics().empty());

    std::cout << "PASS: memory addressing, IR blocks, executable-memory invalidation, cache invalidation, and feature gating.\n";
}
