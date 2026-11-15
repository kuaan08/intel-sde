// SDE AVX Lab — independent AVX1 interpreter with register and memory operands.
// No Intel SDE code is used here. The host backend is scalar C++ only.
#include "avx_runtime/memory_address.hpp"
#include "avx_runtime/block_builder.hpp"
#include "avx_runtime/block_cache.hpp"
#include "avx_runtime/ir.hpp"
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
enum class Op { AddPS, SubPS, MulPS, DivPS, XorPS, AndPS, OrPS, Move, ZeroUpper };
struct Decoded {
    Op op; unsigned dst; unsigned src1; unsigned src2; unsigned bits;
    bool memory = false; bool memory_destination = false; MemoryOperand memory_operand{};
    std::uint64_t guest_address = 0;
};

static void fail(const std::string& s) { throw std::runtime_error(s); }
static std::uint32_t u32(const std::uint8_t* p) { std::uint32_t v; std::memcpy(&v, p, 4); return v; }
static void put32(std::uint8_t* p, std::uint32_t v) { std::memcpy(p, &v, 4); }
static float f32(const std::uint8_t* p) { float v; std::memcpy(&v, p, 4); return v; }
static void putf32(std::uint8_t* p, float v) { std::memcpy(p, &v, 4); }

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

static std::vector<std::uint8_t> parse_hex(const std::string& text) {
    std::string compact;
    for (char c : text) { if (std::isspace(static_cast<unsigned char>(c)) || c == ':') continue; if (!std::isxdigit(static_cast<unsigned char>(c))) fail("invalid hexadecimal character in input"); compact += c; }
    if (compact.size() % 2) fail("hex input must have an even number of digits");
    std::vector<std::uint8_t> out; for (std::size_t i = 0; i < compact.size(); i += 2) out.push_back(static_cast<std::uint8_t>(std::stoul(compact.substr(i, 2), nullptr, 16))); return out;
}

static void seed(VectorState& s) { for (unsigned r = 0; r < 16; ++r) for (unsigned i = 0; i < 8; ++i) putf32(s.ymm[r].data() + i * 4, static_cast<float>(r + i + 1)); }
static void seed(MachineState& state) {
    state.memory.assign(4096, 0); state.gpr.fill(0); state.gpr[0] = 256; state.gpr[1] = 512; seed(state.vectors);
    for (unsigned i = 0; i < 8; ++i) { putf32(state.memory.data() + 256 + i * 4, static_cast<float>(10 + i)); }
}
static std::string state_digest(const VectorState& s) { std::uint32_t hash = 2166136261u; for (const auto& r : s.ymm) for (std::uint8_t b : r) { hash ^= b; hash *= 16777619u; } std::ostringstream out; out << std::hex << std::setfill('0') << std::setw(8) << hash; return out.str(); }

static IROpcode to_ir_opcode(Op op) {
    switch (op) { case Op::AddPS: return IROpcode::AddPS; case Op::SubPS: return IROpcode::SubPS; case Op::MulPS: return IROpcode::MulPS; case Op::DivPS: return IROpcode::DivPS; case Op::XorPS: return IROpcode::XorPS; case Op::AndPS: return IROpcode::AndPS; case Op::OrPS: return IROpcode::OrPS; case Op::Move: return IROpcode::Move; case Op::ZeroUpper: return IROpcode::ZeroUpper; default: return IROpcode::Unsupported; }
}

static IRInstruction to_ir(const Decoded& decoded, std::uint64_t guest_address, std::uint8_t length) {
    IRInstruction instruction; instruction.opcode = to_ir_opcode(decoded.op); instruction.dst = static_cast<std::uint8_t>(decoded.dst); instruction.src1 = static_cast<std::uint8_t>(decoded.src1); instruction.src2 = static_cast<std::uint8_t>(decoded.src2); instruction.vector_bits = static_cast<std::uint16_t>(decoded.bits); instruction.guest_address = guest_address; instruction.length = length; instruction.has_memory_operand = decoded.memory; instruction.memory = decoded.memory_operand; return instruction;
}

static unsigned run_file(const std::string& path) {
    std::ifstream in(path); if (!in) fail("cannot open program file: " + path);
    MachineState state; seed(state); BasicBlockBuilder builder(0x1000); BlockCache cache; std::vector<std::uint8_t> source_bytes; std::string line; unsigned count = 0; std::uint64_t guest_address = 0x1000;
    while (std::getline(in, line)) { std::string trimmed; for (char c : line) if (c != '#') trimmed += c; else break; bool has_hex = false; for (char c : trimmed) if (std::isxdigit(static_cast<unsigned char>(c))) { has_hex = true; break; } if (!has_hex) continue; const std::vector<std::uint8_t> bytes = parse_hex(trimmed); const Decoded decoded = decode(bytes); builder.append(to_ir(decoded, guest_address, static_cast<std::uint8_t>(bytes.size()))); source_bytes.insert(source_bytes.end(), bytes.begin(), bytes.end()); execute(decoded, state); guest_address += bytes.size(); ++count; }
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
    std::cout << "PASS: independent AVX1 interpreter self-test; register and memory operands; no AVX host instructions required.\n";
}

} // namespace avxlab

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--self-test") { avxlab::self_test(); return 0; }
        if (argc == 3 && std::string(argv[1]) == "--benchmark") { avxlab::benchmark(static_cast<unsigned>(std::stoul(argv[2]))); return 0; }
        if (argc == 4 && std::string(argv[1]) == "--run-process") { const int result = avxlab::run_process_with_shim(argv[2], argv[3]); if (result < 0) { std::cerr << "FAIL: Windows process host error " << result << "\n"; return 1; } return result; }
        if (argc == 3 && std::string(argv[1]) == "--decode") { avxlab::Decoded d = avxlab::decode(avxlab::parse_hex(argv[2])); std::cout << "decoded bits=" << d.bits << " dst=ymm" << d.dst << " src1=ymm" << d.src1; if (d.memory) std::cout << " src2=" << avxlab::format_memory_operand(d.memory_operand); else std::cout << " src2=ymm" << d.src2; std::cout << " memory=" << (d.memory ? "yes" : "no") << "\n"; return 0; }
        if (argc == 3 && std::string(argv[1]) == "--run-file") { avxlab::run_file(argv[2]); return 0; }
        std::cerr << "usage: avx_runtime --self-test | --decode \"c5 f0 58 c2\" | --run-file program.txt | --benchmark iterations | --run-process target.exe shim.dll\n"; return 64;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
