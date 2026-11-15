// SDE AVX Lab — independent AVX1 register-only interpreter.
// No Intel SDE code is used here. The host backend is scalar C++ only.
#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace avxlab {

struct VectorState { std::array<std::array<std::uint8_t, 32>, 16> ymm{}; };
enum class Op { AddPS, SubPS, MulPS, DivPS, XorPS, AndPS, OrPS, Move, ZeroUpper };
struct Decoded { Op op; unsigned dst; unsigned src1; unsigned src2; unsigned bits; };

static void fail(const std::string& s) { throw std::runtime_error(s); }
static std::uint32_t u32(const std::uint8_t* p) { std::uint32_t v; std::memcpy(&v, p, 4); return v; }
static void put32(std::uint8_t* p, std::uint32_t v) { std::memcpy(p, &v, 4); }
static float f32(const std::uint8_t* p) { float v; std::memcpy(&v, p, 4); return v; }
static void putf32(std::uint8_t* p, float v) { std::memcpy(p, &v, 4); }

Decoded decode(const std::vector<std::uint8_t>& b) {
    if (b.size() < 3) fail("truncated VEX instruction");
    std::size_t off = 0; unsigned map = 0, pp = 0, w = 0, l = 0, v = 0, r = 0, bx = 0;
    if (b[0] == 0xC5) {
        std::uint8_t p = b[1]; map = 1; r = ((p >> 7) & 1U) ^ 1U; v = (p >> 3) & 15U; l = (p >> 2) & 1U; pp = p & 3U; off = 2;
    } else if (b[0] == 0xC4) {
        if (b.size() < 4) fail("truncated VEX3 instruction");
        std::uint8_t p0 = b[1], p1 = b[2]; map = p0 & 31U; r = ((p0 >> 7) & 1U) ^ 1U; bx = ((p0 >> 5) & 1U) ^ 1U; w = (p1 >> 7) & 1U; v = (p1 >> 3) & 15U; l = (p1 >> 2) & 1U; pp = p1 & 3U; off = 3;
    } else fail("not a VEX instruction");
    if (map != 1 || pp != 0 || w != 0 || off >= b.size()) fail("unsupported VEX form");
    if (b[off] == 0x77) {
        if (l || v != 15 || b.size() != off + 1) fail("invalid VZEROUPPER");
        return {Op::ZeroUpper, 0, 0, 0, 256};
    }
    if (b.size() != off + 2 || (b[off + 1] >> 6) != 3) fail("only register operands are supported");
    unsigned reg = ((b[off + 1] >> 3) & 7U) | (r << 3); unsigned rm = (b[off + 1] & 7U) | (bx << 3); unsigned src1 = (~v) & 15U; unsigned bits = l ? 256 : 128;
    Op op;
    switch (b[off]) {
    case 0x58: op = Op::AddPS; break; case 0x5C: op = Op::SubPS; break; case 0x59: op = Op::MulPS; break; case 0x5E: op = Op::DivPS; break;
    case 0x57: op = Op::XorPS; break; case 0x54: op = Op::AndPS; break; case 0x56: op = Op::OrPS; break;
    case 0x10: case 0x28: if (v != 15) fail("invalid VMOV reserved vvvv"); op = Op::Move; break;
    default: fail("unsupported AVX1 opcode");
    }
    if (op == Op::Move) return {op, reg, 0, rm, bits};
    return {op, reg, src1, rm, bits};
}

void execute(const Decoded& d, VectorState& s) {
    if (d.op == Op::ZeroUpper) { for (auto& r : s.ymm) std::memset(r.data() + 16, 0, 16); return; }
    unsigned bytes = d.bits / 8;
    if (d.op == Op::Move) { std::memcpy(s.ymm[d.dst].data(), s.ymm[d.src2].data(), bytes); if (d.bits == 128) std::memset(s.ymm[d.dst].data() + 16, 0, 16); return; }
    for (unsigned i = 0; i < bytes; i += 4) {
        std::uint32_t a = u32(s.ymm[d.src1].data() + i), b = u32(s.ymm[d.src2].data() + i);
        switch (d.op) {
        case Op::AddPS: putf32(s.ymm[d.dst].data() + i, f32(s.ymm[d.src1].data() + i) + f32(s.ymm[d.src2].data() + i)); break;
        case Op::SubPS: putf32(s.ymm[d.dst].data() + i, f32(s.ymm[d.src1].data() + i) - f32(s.ymm[d.src2].data() + i)); break;
        case Op::MulPS: putf32(s.ymm[d.dst].data() + i, f32(s.ymm[d.src1].data() + i) * f32(s.ymm[d.src2].data() + i)); break;
        case Op::DivPS: putf32(s.ymm[d.dst].data() + i, f32(s.ymm[d.src1].data() + i) / f32(s.ymm[d.src2].data() + i)); break;
        case Op::XorPS: put32(s.ymm[d.dst].data() + i, a ^ b); break; case Op::AndPS: put32(s.ymm[d.dst].data() + i, a & b); break; case Op::OrPS: put32(s.ymm[d.dst].data() + i, a | b); break;
        default: fail("internal unsupported operation");
        }
    }
    if (d.bits == 128) std::memset(s.ymm[d.dst].data() + 16, 0, 16);
}

static std::vector<std::uint8_t> parse_hex(const std::string& text) {
    std::string compact; for (char c : text) if (c != ' ' && c != '\t' && c != ':') compact += c;
    if (compact.size() % 2) fail("hex input must have an even number of digits");
    std::vector<std::uint8_t> out;
    for (std::size_t i = 0; i < compact.size(); i += 2)
        out.push_back(static_cast<std::uint8_t>(std::stoul(compact.substr(i, 2), nullptr, 16)));
    return out;
}

static void seed(VectorState& s) { for (unsigned r = 0; r < 16; ++r) for (unsigned i = 0; i < 8; ++i) putf32(s.ymm[r].data() + i * 4, static_cast<float>(r + i + 1)); }
static std::string state_digest(const VectorState& s) {
    std::uint32_t hash = 2166136261u;
    for (const auto& r : s.ymm) for (std::uint8_t b : r) { hash ^= b; hash *= 16777619u; }
    std::ostringstream out; out << std::hex << std::setfill('0') << std::setw(8) << hash; return out.str();
}

static unsigned run_file(const std::string& path) {
    std::ifstream in(path); if (!in) fail("cannot open program file: " + path);
    VectorState state; seed(state); std::string line; unsigned count = 0;
    while (std::getline(in, line)) {
        std::string trimmed; for (char c : line) if (c != '#') trimmed += c; else break;
        bool has_hex = false; for (char c : trimmed) if (std::isxdigit(static_cast<unsigned char>(c))) { has_hex = true; break; }
        if (!has_hex) continue;
        execute(decode(parse_hex(trimmed)), state); ++count;
    }
    std::cout << "PASS: executed " << count << " independent VEX instructions from " << path << "\n";
    std::cout << "state_digest=" << state_digest(state) << " xmm0=";
    for (unsigned i = 0; i < 4; ++i) { if (i) std::cout << ','; std::cout << f32(state.ymm[0].data() + i * 4); }
    std::cout << "\n"; return count;
}

static void self_test() {
    VectorState s; seed(s); execute(decode(parse_hex("c5 f0 58 c2")), s); // vaddps xmm0,xmm1,xmm2
    if (f32(s.ymm[0].data()) != 5.0f) fail("VADDPS result mismatch");
    execute(decode(parse_hex("c5 f8 77")), s); for (auto& r : s.ymm) for (unsigned i = 16; i < 32; ++i) if (r[i] != 0) fail("VZEROUPPER mismatch");
    std::cout << "PASS: independent AVX1 interpreter self-test; no AVX host instructions required.\n";
}

} // namespace avxlab

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--self-test") { avxlab::self_test(); return 0; }
        if (argc == 3 && std::string(argv[1]) == "--decode") { avxlab::Decoded d = avxlab::decode(avxlab::parse_hex(argv[2])); std::cout << "decoded bits=" << d.bits << " dst=ymm" << d.dst << " src1=ymm" << d.src1 << " src2=ymm" << d.src2 << "\n"; return 0; }
        if (argc == 3 && std::string(argv[1]) == "--run-file") return avxlab::run_file(argv[2]) == 0 ? 1 : 0;
        std::cerr << "usage: avx_runtime --self-test | --decode \"c5 f0 58 c2\" | --run-file program.txt\n"; return 64;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
