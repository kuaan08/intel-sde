// SDE AVX Lab — independent AVX1 interpreter with register and memory operands.
// No Intel SDE code is used here. The host backend is scalar C++ only.
#include "avx_runtime/memory_address.hpp"
#include "avx_runtime/block_builder.hpp"
#include "avx_runtime/block_cache.hpp"
#include "avx_runtime/decoder.hpp"
#include "avx_runtime/ir.hpp"
#include "avx_runtime/translator.hpp"
#include "avx_runtime/windows_process_host.hpp"
#include <array>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace avxlab {

struct VectorState { std::array<std::array<std::uint8_t, 32>, 16> ymm{}; };
struct MachineState {
    VectorState vectors;
    std::array<std::uint64_t, 16> gpr{};
    std::vector<std::uint8_t> memory;
};

static void fail(const std::string& s) { throw std::runtime_error(s); }
static std::uint32_t u32(const std::uint8_t* p) { std::uint32_t v; std::memcpy(&v, p, 4); return v; }
static void put32(std::uint8_t* p, std::uint32_t v) { std::memcpy(p, &v, 4); }
static float f32(const std::uint8_t* p) { float v; std::memcpy(&v, p, 4); return v; }
static void putf32(std::uint8_t* p, float v) { std::memcpy(p, &v, 4); }

static std::uint64_t effective_address(const Decoded& d, const MachineState& state) {
    std::uint64_t address = d.memory_operand.rip_relative ? d.guest_address + d.memory_operand.encoded_length : 0;
    if (d.memory_operand.has_base) address += state.gpr[d.memory_operand.base];
    if (d.memory_operand.has_index) {
        const std::uint64_t index = state.gpr[d.memory_operand.index];
        if (d.memory_operand.scale != 0 && index > (std::numeric_limits<std::uint64_t>::max() / d.memory_operand.scale)) fail("x64 address overflow");
        address += index * d.memory_operand.scale;
    }
    if (d.memory_operand.displacement >= 0) address += static_cast<std::uint64_t>(d.memory_operand.displacement);
    else {
        const std::uint64_t amount = static_cast<std::uint64_t>(-d.memory_operand.displacement);
        if (address < amount) fail("x64 address underflow");
        address -= amount;
    }
    return address;
}

static std::uint8_t* memory_ptr(MachineState& state, std::uint64_t address, unsigned bytes) {
    if (address > state.memory.size() || bytes > state.memory.size() - static_cast<std::size_t>(address)) fail("guest memory access out of range");
    return state.memory.data() + static_cast<std::size_t>(address);
}

static void execute_impl(const Decoded& d, VectorState& vectors, MachineState* machine) {
    if (d.op == Op::ZeroUpper) { for (auto& r : vectors.ymm) std::memset(r.data() + 16, 0, 16); return; }
    const unsigned bytes = d.bits / 8;
    const std::uint8_t* source2 = nullptr;
    std::uint8_t* destination = nullptr;
    if (d.memory) {
        if (machine == nullptr) fail("memory operand requires MachineState");
        const std::uint64_t address = effective_address(d, *machine);
        std::uint8_t* memory = memory_ptr(*machine, address, bytes);
        if (d.memory_destination) {
            std::memcpy(memory, vectors.ymm[d.src2].data(), bytes);
            return;
        }
        source2 = memory;
    } else {
        source2 = vectors.ymm[d.src2].data();
    }
    if (d.op == Op::Move) {
        destination = vectors.ymm[d.dst].data();
        std::memcpy(destination, source2, bytes);
        if (d.bits == 128) std::memset(destination + 16, 0, 16);
        return;
    }
    for (unsigned i = 0; i < bytes; i += 4) {
        const std::uint32_t a = u32(vectors.ymm[d.src1].data() + i), b = u32(source2 + i);
        switch (d.op) {
        case Op::AddPS: putf32(vectors.ymm[d.dst].data() + i, f32(vectors.ymm[d.src1].data() + i) + f32(source2 + i)); break;
        case Op::SubPS: putf32(vectors.ymm[d.dst].data() + i, f32(vectors.ymm[d.src1].data() + i) - f32(source2 + i)); break;
        case Op::MulPS: putf32(vectors.ymm[d.dst].data() + i, f32(vectors.ymm[d.src1].data() + i) * f32(source2 + i)); break;
        case Op::DivPS: putf32(vectors.ymm[d.dst].data() + i, f32(vectors.ymm[d.src1].data() + i) / f32(source2 + i)); break;
        case Op::XorPS: put32(vectors.ymm[d.dst].data() + i, a ^ b); break;
        case Op::AndPS: put32(vectors.ymm[d.dst].data() + i, a & b); break;
        case Op::OrPS: put32(vectors.ymm[d.dst].data() + i, a | b); break;
        default: fail("internal unsupported operation");
        }
    }
    if (d.bits == 128) std::memset(vectors.ymm[d.dst].data() + 16, 0, 16);
}

void execute(const Decoded& d, VectorState& state) { execute_impl(d, state, nullptr); }
void execute(const Decoded& d, MachineState& state) { execute_impl(d, state.vectors, &state); }

static void seed(VectorState& s) { for (unsigned r = 0; r < 16; ++r) for (unsigned i = 0; i < 8; ++i) putf32(s.ymm[r].data() + i * 4, static_cast<float>(r + i + 1)); }
static void seed(MachineState& state) {
    state.memory.assign(4096, 0); state.gpr.fill(0); state.gpr[0] = 256; state.gpr[1] = 512; seed(state.vectors);
    for (unsigned i = 0; i < 8; ++i) { putf32(state.memory.data() + 256 + i * 4, static_cast<float>(10 + i)); }
}
static std::string state_digest(const VectorState& s) { std::uint32_t hash = 2166136261u; for (const auto& r : s.ymm) for (std::uint8_t b : r) { hash ^= b; hash *= 16777619u; } std::ostringstream out; out << std::hex << std::setfill('0') << std::setw(8) << hash; return out.str(); }

static unsigned run_file(const std::string& path) {
    std::ifstream in(path); if (!in) fail("cannot open program file: " + path);
    MachineState state; seed(state); BasicBlockBuilder builder(0x1000); BlockCache cache; std::vector<std::uint8_t> source_bytes; std::string line; unsigned count = 0; std::uint64_t guest_address = 0x1000;
    while (std::getline(in, line)) { std::string trimmed; for (char c : line) if (c != '#') trimmed += c; else break; bool has_hex = false; for (char c : trimmed) if (std::isxdigit(static_cast<unsigned char>(c))) { has_hex = true; break; } if (!has_hex) continue; const std::vector<std::uint8_t> bytes = parse_hex(trimmed); const Decoded decoded = decode(bytes); builder.append(make_ir_instruction(decoded, guest_address, static_cast<std::uint8_t>(bytes.size()))); source_bytes.insert(source_bytes.end(), bytes.begin(), bytes.end()); execute(decoded, state); guest_address += bytes.size(); ++count; }
    IRBlock block = builder.finish(); block.generation = 1; cache.insert(block, source_bytes);
    std::cout << "PASS: executed " << count << " independent VEX instructions from " << path << "\n";
    std::cout << "ir_block.instructions=" << block.instructions.size() << " cache_entries=" << cache.size() << "\n";
    std::cout << "state_digest=" << state_digest(state.vectors) << " xmm0="; for (unsigned i = 0; i < 4; ++i) { if (i) std::cout << ','; std::cout << f32(state.vectors.ymm[0].data() + i * 4); } std::cout << "\n"; return count;
}

static void benchmark(unsigned iterations) {
    if (iterations == 0) fail("benchmark iterations must be non-zero");
    MachineState state; seed(state);
    const Decoded register_add = decode(parse_hex("c5 f0 58 c2"));
    const Decoded memory_add = decode(parse_hex("c5 f0 58 00"));
    const auto started = std::chrono::steady_clock::now();
    for (unsigned i = 0; i < iterations; ++i) { execute(register_add, state); execute(memory_add, state); }
    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    const double operations = static_cast<double>(iterations) * 2.0;
    std::cout << "benchmark.operations=" << static_cast<std::uint64_t>(operations) << " seconds=" << elapsed << " ops_per_second=" << (operations / elapsed) << " digest=" << state_digest(state.vectors) << "\n";
}

static void self_test() {
    VectorState s; seed(s); execute(decode(parse_hex("c5 f0 58 c2")), s); if (f32(s.ymm[0].data()) != 5.0f) fail("VADDPS result mismatch");
    MachineState memory; seed(memory); execute(decode(parse_hex("c5 f0 58 00")), memory); if (f32(memory.vectors.ymm[0].data()) != 12.0f) fail("memory VADDPS result mismatch");
    execute(decode(parse_hex("c5 f8 11 01")), memory); if (f32(memory.memory.data() + 512) != 12.0f) fail("memory VMOV store mismatch");
    execute(decode(parse_hex("c5 f8 77")), s); for (auto& r : s.ymm) for (unsigned i = 16; i < 32; ++i) if (r[i] != 0) fail("VZEROUPPER mismatch");
    const IRInstruction register_ir = make_ir_instruction(decode(parse_hex("c5 f0 58 c2")), 0x1000, 4);
    if (register_ir.access.ymm_read_mask != ((1U << 1) | (1U << 2)) ||
        register_ir.access.ymm_write_mask != (1U << 0) ||
        register_ir.memory_access != MemoryAccess::None ||
        register_ir.exception_behavior != ExceptionBehavior::MayRaiseFloatingPoint)
        fail("IR read/write metadata mismatch");
    const IRInstruction memory_ir = make_ir_instruction(decode(parse_hex("c5 f0 58 00")), 0x1000, 4);
    if (memory_ir.memory_access != MemoryAccess::Read || memory_ir.memory_width_bytes != 16 ||
        memory_ir.exception_behavior != ExceptionBehavior::MayFaultOnMemory)
        fail("IR memory metadata mismatch");

    const std::vector<std::vector<std::uint8_t>> invalid = {
        {}, {0xC5}, {0xC4, 0xE1}, {0x90, 0x90, 0x90},
        {0xC5, 0xF0, 0x58}, {0xC5, 0xF0, 0xFA, 0xC2},
        {0xC5, 0xF0, 0x10, 0xC2}, {0xC5, 0xF0, 0x58, 0x04, 0x24, 0x00},
        {0xC5, 0xF8, 0x77, 0x90}
    };
    for (const auto& bytes : invalid) {
        bool rejected = false;
        try { (void)decode(bytes); } catch (const std::exception&) { rejected = true; }
        if (!rejected) fail("decoder accepted invalid encoding");
    }
    std::cout << "PASS: independent AVX1 interpreter self-test; IR metadata; negative decoder tests; no AVX host instructions required.\n";
}

} // namespace avxlab

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--self-test") { avxlab::self_test(); return 0; }
        if (argc == 3 && std::string(argv[1]) == "--benchmark") { avxlab::benchmark(static_cast<unsigned>(std::stoul(argv[2]))); return 0; }
        if (argc >= 4 && std::string(argv[1]) == "--run-process") { std::string target_arguments; for (int i = 4; i < argc; ++i) { if (!target_arguments.empty()) target_arguments += " "; target_arguments += argv[i]; } const int result = avxlab::run_process_with_shim(argv[2], argv[3], target_arguments); const std::uint32_t code = static_cast<std::uint32_t>(result); if (code == 0xC000001DU) std::cerr << "FAIL: target stopped with STATUS_ILLEGAL_INSTRUCTION (0xC000001D); the shim encountered an unsupported instruction.\n"; else if (result == -11) std::cerr << "FAIL: invalid target path or UTF-8 path conversion.\n"; else if (result == -12) std::cerr << "FAIL: invalid shim path or UTF-8 path conversion.\n"; else if (result == -13) std::cerr << "FAIL: target executable was not found.\n"; else if (result == -14) std::cerr << "FAIL: avx_winshim.dll was not found.\n"; else if (result == -1) std::cerr << "FAIL: empty target or shim path; use the current Windows package and provide both files.\n"; else if (result < 0) std::cerr << "FAIL: Windows process host error " << result << "\n"; return result < 0 ? 1 : result; }
        if (argc == 3 && std::string(argv[1]) == "--decode") { avxlab::Decoded d = avxlab::decode(avxlab::parse_hex(argv[2])); std::cout << "decoded bits=" << d.bits << " dst=ymm" << d.dst << " src1=ymm" << d.src1; if (d.memory) std::cout << " src2=" << avxlab::format_memory_operand(d.memory_operand); else std::cout << " src2=ymm" << d.src2; std::cout << " memory=" << (d.memory ? "yes" : "no") << "\n"; return 0; }
        if (argc == 3 && std::string(argv[1]) == "--run-file") { avxlab::run_file(argv[2]); return 0; }
        std::cerr << "usage: avx_runtime --self-test | --decode \"c5 f0 58 c2\" | --run-file program.txt | --benchmark iterations | --run-process target.exe shim.dll [target arguments]\n"; return 64;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
