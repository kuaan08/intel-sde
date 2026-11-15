// SDE AVX Lab — narrow AVX1 -> legacy SSE proof of concept.
// Supported forms: register-register VEX.128/VEX.256 packed-single arithmetic,
// bitwise logical operations, register-register VMOVUPS/VMOVAPS, and VZEROUPPER.
// This is a test harness, not a process-level DBT runtime.

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#if defined(_MSC_VER)
#include <intrin.h>
#endif
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#include <cpuid.h>
#endif

namespace {

constexpr unsigned kGuestVectorRegisters = 16;
constexpr unsigned kBytesPerYmm = 32;

struct alignas(32) VectorState {
    std::uint8_t ymm[kGuestVectorRegisters][kBytesPerYmm];
};

enum class Op {
    AddPS,
    SubPS,
    MulPS,
    DivPS,
    XorPS,
    AndPS,
    OrPS,
    Move,
    ZeroUpper
};

struct Decoded {
    Op op{};
    unsigned dst{};
    unsigned src1{};
    unsigned src2{};
    unsigned vector_bits{};
    std::size_t length{};
};

struct VexFields {
    unsigned map{};
    unsigned pp{};
    unsigned w{};
    unsigned l{};
    unsigned vvvv_encoded{};
    unsigned r_ext{};
    unsigned x_ext{};
    unsigned b_ext{};
    std::size_t opcode_offset{};
};

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message);
}

VexFields parse_vex(const std::vector<std::uint8_t>& bytes) {
    if (bytes.empty()) fail("empty instruction");

    VexFields f{};
    const std::uint8_t first = bytes[0];
    if (first == 0xC5) {
        if (bytes.size() < 3) fail("truncated two-byte VEX instruction");
        const std::uint8_t p1 = bytes[1];
        f.map = 1; // C5 is always the 0F opcode map.
        f.r_ext = ((p1 >> 7) & 1U) ^ 1U;
        f.x_ext = 0;
        f.b_ext = 0;
        f.vvvv_encoded = (p1 >> 3) & 0x0FU;
        f.l = (p1 >> 2) & 1U;
        f.pp = p1 & 3U;
        f.w = 0;
        f.opcode_offset = 2;
    } else if (first == 0xC4) {
        if (bytes.size() < 4) fail("truncated three-byte VEX instruction");
        const std::uint8_t p0 = bytes[1];
        const std::uint8_t p1 = bytes[2];
        f.map = p0 & 0x1FU;
        f.r_ext = ((p0 >> 7) & 1U) ^ 1U;
        f.x_ext = ((p0 >> 6) & 1U) ^ 1U;
        f.b_ext = ((p0 >> 5) & 1U) ^ 1U;
        f.w = (p1 >> 7) & 1U;
        f.vvvv_encoded = (p1 >> 3) & 0x0FU;
        f.l = (p1 >> 2) & 1U;
        f.pp = p1 & 3U;
        f.opcode_offset = 3;
    } else {
        fail("only VEX2 (C5) and VEX3 (C4) encodings are accepted");
    }
    return f;
}

Decoded decode_one(const std::vector<std::uint8_t>& bytes) {
    const VexFields vex = parse_vex(bytes);
    if (vex.map != 1 || vex.pp != 0 || vex.w != 0)
        fail("only VEX map 0F, pp=0, W=0 forms are accepted");
    if (vex.opcode_offset >= bytes.size()) fail("missing opcode");

    const std::uint8_t opcode = bytes[vex.opcode_offset];
    if (opcode == 0x77) { // VZEROUPPER (L=0); VZEROALL (L=1) is deliberately rejected.
        if (vex.l != 0 || vex.vvvv_encoded != 0x0F)
            fail("only the reserved-field-valid VZEROUPPER form is accepted");
        if (bytes.size() != vex.opcode_offset + 1)
            fail("trailing bytes after VZEROUPPER");
        return Decoded{Op::ZeroUpper, 0, 0, 0, 256, bytes.size()};
    }

    if (bytes.size() != vex.opcode_offset + 2)
        fail("expected exactly one VEX instruction with a ModRM byte");
    const std::uint8_t modrm = bytes[vex.opcode_offset + 1];
    const unsigned mod = modrm >> 6;
    if (mod != 3) fail("memory operands are not supported by this POC");
    if (vex.x_ext != 0) fail("reserved VEX.X extension on register-only form");

    const unsigned reg = ((modrm >> 3) & 7U) | (vex.r_ext << 3);
    const unsigned rm = (modrm & 7U) | (vex.b_ext << 3);
    const unsigned src1 = (~vex.vvvv_encoded) & 0x0FU;
    const unsigned width = vex.l ? 256 : 128;

    Decoded d{};
    d.vector_bits = width;
    d.length = bytes.size();

    switch (opcode) {
    case 0x58: d.op = Op::AddPS; d.dst = reg; d.src1 = src1; d.src2 = rm; break;
    case 0x5C: d.op = Op::SubPS; d.dst = reg; d.src1 = src1; d.src2 = rm; break;
    case 0x59: d.op = Op::MulPS; d.dst = reg; d.src1 = src1; d.src2 = rm; break;
    case 0x5E: d.op = Op::DivPS; d.dst = reg; d.src1 = src1; d.src2 = rm; break;
    case 0x57: d.op = Op::XorPS; d.dst = reg; d.src1 = src1; d.src2 = rm; break;
    case 0x54: d.op = Op::AndPS; d.dst = reg; d.src1 = src1; d.src2 = rm; break;
    case 0x56: d.op = Op::OrPS;  d.dst = reg; d.src1 = src1; d.src2 = rm; break;
    case 0x10: // VMOVUPS/VMOVAPS xmm/ymm, xmm/ymm
    case 0x28: // aligned spelling has the same register-register data movement
        if (vex.vvvv_encoded != 0x0F) fail("VMOV load form requires reserved VEX.vvvv=1111b");
        d.op = Op::Move; d.dst = reg; d.src2 = rm; break;
    case 0x11: // VMOVUPS/VMOVAPS xmm/ymm, xmm/ymm (destination is r/m)
    case 0x29:
        if (vex.vvvv_encoded != 0x0F) fail("VMOV store form requires reserved VEX.vvvv=1111b");
        d.op = Op::Move; d.dst = rm; d.src2 = reg; break;
    default:
        fail("unsupported AVX opcode in this POC");
    }

    if (d.dst >= kGuestVectorRegisters || d.src1 >= kGuestVectorRegisters ||
        d.src2 >= kGuestVectorRegisters)
        fail("decoded vector register is out of range");
    return d;
}

class Emitter {
public:
    std::vector<std::uint8_t> code;

    void byte(std::uint8_t b) { code.push_back(b); }
    void dword(std::uint32_t v) {
        byte(static_cast<std::uint8_t>(v));
        byte(static_cast<std::uint8_t>(v >> 8));
        byte(static_cast<std::uint8_t>(v >> 16));
        byte(static_cast<std::uint8_t>(v >> 24));
    }

    void load_xmm(unsigned xmm, unsigned offset) {
        // MOVUPS xmm, [state + disp32]. The state pointer is arg1:
        // RDI under SysV AMD64; RCX under Windows x64.
#if defined(_WIN32)
        constexpr unsigned base = 1; // RCX
#else
        constexpr unsigned base = 7; // RDI
#endif
        byte(0x0F); byte(0x10);
        byte(static_cast<std::uint8_t>(0x80 | ((xmm & 7U) << 3) | base));
        dword(offset);
    }

    void store_xmm(unsigned offset, unsigned xmm) {
#if defined(_WIN32)
        constexpr unsigned base = 1; // RCX
#else
        constexpr unsigned base = 7; // RDI
#endif
        byte(0x0F); byte(0x11);
        byte(static_cast<std::uint8_t>(0x80 | ((xmm & 7U) << 3) | base));
        dword(offset);
    }

    void zero_xmm0() { byte(0x0F); byte(0x57); byte(0xC0); } // XORPS xmm0,xmm0

    void op_xmm0_xmm1(Op op) {
        std::uint8_t opcode = 0;
        switch (op) {
        case Op::AddPS: opcode = 0x58; break;
        case Op::SubPS: opcode = 0x5C; break;
        case Op::MulPS: opcode = 0x59; break;
        case Op::DivPS: opcode = 0x5E; break;
        case Op::XorPS: opcode = 0x57; break;
        case Op::AndPS: opcode = 0x54; break;
        case Op::OrPS:  opcode = 0x56; break;
        default: fail("internal error: operation has no SSE lowering");
        }
        // Legacy two-operand SSE: xmm0 := xmm0 OP xmm1.
        byte(0x0F); byte(opcode); byte(0xC1);
    }
};

std::vector<std::uint8_t> translate(const std::vector<std::uint8_t>& bytes) {
    const Decoded d = decode_one(bytes);
    Emitter e;
    if (d.op == Op::ZeroUpper) {
        e.zero_xmm0();
        for (unsigned reg = 0; reg < kGuestVectorRegisters; ++reg)
            e.store_xmm(reg * kBytesPerYmm + 16, 0);
    } else if (d.op == Op::Move) {
        const unsigned halves = d.vector_bits == 256 ? 2 : 1;
        for (unsigned half = 0; half < halves; ++half) {
            const unsigned offset = half * 16;
            e.load_xmm(0, d.src2 * kBytesPerYmm + offset);
            e.store_xmm(d.dst * kBytesPerYmm + offset, 0);
        }
        if (d.vector_bits == 128) {
            e.zero_xmm0();
            e.store_xmm(d.dst * kBytesPerYmm + 16, 0);
        }
    } else {
        const unsigned halves = d.vector_bits == 256 ? 2 : 1;
        for (unsigned half = 0; half < halves; ++half) {
            const unsigned offset = half * 16;
            e.load_xmm(0, d.src1 * kBytesPerYmm + offset);
            e.load_xmm(1, d.src2 * kBytesPerYmm + offset);
            e.op_xmm0_xmm1(d.op);
            e.store_xmm(d.dst * kBytesPerYmm + offset, 0);
        }
        if (d.vector_bits == 128) {
            e.zero_xmm0();
            e.store_xmm(d.dst * kBytesPerYmm + 16, 0);
        }
    }
    e.byte(0xC3); // RET
    return e.code;
}

class ExecutableCode {
public:
    explicit ExecutableCode(const std::vector<std::uint8_t>& code) : size_(code.size()) {
#if defined(_WIN32)
        memory_ = VirtualAlloc(nullptr, size_, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!memory_) fail("VirtualAlloc failed");
        std::memcpy(memory_, code.data(), code.size());
        DWORD old_protect = 0;
        if (!VirtualProtect(memory_, size_, PAGE_EXECUTE_READ, &old_protect)) {
            VirtualFree(memory_, 0, MEM_RELEASE);
            fail("VirtualProtect failed");
        }
        FlushInstructionCache(GetCurrentProcess(), memory_, size_);
#else
        memory_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (memory_ == MAP_FAILED) fail("mmap failed");
        std::memcpy(memory_, code.data(), code.size());
        if (mprotect(memory_, size_, PROT_READ | PROT_EXEC) != 0) {
            munmap(memory_, size_);
            fail("mprotect failed");
        }
        __builtin___clear_cache(static_cast<char*>(memory_), static_cast<char*>(memory_) + size_);
#endif
    }

    ~ExecutableCode() {
#if defined(_WIN32)
        if (memory_) VirtualFree(memory_, 0, MEM_RELEASE);
#else
        if (memory_ && memory_ != MAP_FAILED) munmap(memory_, size_);
#endif
    }

    ExecutableCode(const ExecutableCode&) = delete;
    ExecutableCode& operator=(const ExecutableCode&) = delete;

    void run(VectorState& state) const {
        using Function = void (*)(VectorState*);
        auto function = reinterpret_cast<Function>(memory_);
        function(&state);
    }

private:
    void* memory_{};
    std::size_t size_{};
};

// Test-only encoding helper. The decoder itself consumes instruction bytes and
// does not use these values as decoded IR.
std::vector<std::uint8_t> make_vex(unsigned opcode, bool l, unsigned modrm_reg,
                                  unsigned modrm_rm, unsigned logical_vvvv,
                                  bool reserved_vvvv = false) {
    const unsigned enc_v = reserved_vvvv ? 0x0FU : ((~logical_vvvv) & 0x0FU);
    const std::uint8_t modrm = static_cast<std::uint8_t>(0xC0 | ((modrm_reg & 7U) << 3) | (modrm_rm & 7U));
    const bool needs_vex3 = modrm_reg >= 8 || modrm_rm >= 8;
    std::vector<std::uint8_t> b;
    if (!needs_vex3) {
        const std::uint8_t p1 = static_cast<std::uint8_t>((modrm_reg < 8 ? 0x80 : 0) |
                                                          (enc_v << 3) | (l ? 0x04 : 0));
        b = {0xC5, p1, static_cast<std::uint8_t>(opcode), modrm};
    } else {
        // VEX map 0F, W=0, X'=1 (unused for register operands).
        const std::uint8_t p0 = static_cast<std::uint8_t>((modrm_reg < 8 ? 0x80 : 0) |
                                                          0x40 |
                                                          (modrm_rm < 8 ? 0x20 : 0) | 0x01);
        const std::uint8_t p1 = static_cast<std::uint8_t>((enc_v << 3) | (l ? 0x04 : 0));
        b = {0xC4, p0, p1, static_cast<std::uint8_t>(opcode), modrm};
    }
    return b;
}

std::vector<std::uint8_t> make_zero_upper() { return {0xC5, 0xF8, 0x77}; }

float read_float(const std::uint8_t* p) {
    float v;
    std::memcpy(&v, p, sizeof(v));
    return v;
}
void write_float(std::uint8_t* p, float v) { std::memcpy(p, &v, sizeof(v)); }
std::uint32_t read_u32(const std::uint8_t* p) {
    std::uint32_t v;
    std::memcpy(&v, p, sizeof(v));
    return v;
}
void write_u32(std::uint8_t* p, std::uint32_t v) { std::memcpy(p, &v, sizeof(v)); }

void seed_state(VectorState& state) {
    std::memset(&state, 0, sizeof(state));
    for (unsigned reg = 0; reg < kGuestVectorRegisters; ++reg) {
        for (unsigned lane = 0; lane < 8; ++lane) {
            const float value = static_cast<float>((static_cast<int>(reg) - 6) * 4 + static_cast<int>(lane) + 1) * 0.25F;
            write_float(&state.ymm[reg][lane * 4], value);
        }
    }
}

float apply_float(Op op, float a, float b) {
    switch (op) {
    case Op::AddPS: return a + b;
    case Op::SubPS: return a - b;
    case Op::MulPS: return a * b;
    case Op::DivPS: return a / b;
    default: fail("not a floating-point operation");
    }
}

std::uint8_t opcode_for(Op op) {
    switch (op) {
    case Op::AddPS: return 0x58;
    case Op::SubPS: return 0x5C;
    case Op::MulPS: return 0x59;
    case Op::DivPS: return 0x5E;
    case Op::XorPS: return 0x57;
    case Op::AndPS: return 0x54;
    case Op::OrPS:  return 0x56;
    default: fail("operation has no VEX opcode");
    }
}

void set_expected_vector(VectorState& expected, const VectorState& before,
                         const Decoded& d) {
    const unsigned bytes = d.vector_bits / 8;
    if (d.op == Op::Move) {
        std::memcpy(&expected.ymm[d.dst][0], &before.ymm[d.src2][0], bytes);
        if (d.vector_bits == 128)
            std::memset(&expected.ymm[d.dst][16], 0, 16);
        return;
    }
    for (unsigned offset = 0; offset < bytes; offset += 4) {
        const std::uint32_t a = read_u32(&before.ymm[d.src1][offset]);
        const std::uint32_t b = read_u32(&before.ymm[d.src2][offset]);
        std::uint32_t result = 0;
        switch (d.op) {
        case Op::AddPS: case Op::SubPS: case Op::MulPS: case Op::DivPS:
            write_float(&expected.ymm[d.dst][offset], apply_float(d.op,
                read_float(&before.ymm[d.src1][offset]), read_float(&before.ymm[d.src2][offset])));
            continue;
        case Op::XorPS: result = a ^ b; break;
        case Op::AndPS: result = a & b; break;
        case Op::OrPS:  result = a | b; break;
        default: fail("unexpected operation in vector reference");
        }
        write_u32(&expected.ymm[d.dst][offset], result);
    }
    if (d.vector_bits == 128)
        std::memset(&expected.ymm[d.dst][16], 0, 16);
}

void check_equal(const VectorState& actual, const VectorState& expected, const std::string& name) {
    if (std::memcmp(&actual, &expected, sizeof(actual)) != 0)
        fail("state mismatch: " + name);
}

unsigned tests_run = 0;
void run_translation_case(const std::vector<std::uint8_t>& instruction,
                         const std::string& label,
                         std::vector<std::uint8_t>* capture = nullptr) {
    const Decoded d = decode_one(instruction);
    const std::vector<std::uint8_t> code = translate(instruction);
    VectorState actual{}, before{}, expected{};
    seed_state(actual);
    before = actual;
    expected = before;
    set_expected_vector(expected, before, d);
    if (capture) *capture = code;
    ExecutableCode executable(code);
    executable.run(actual);
    check_equal(actual, expected, label);
    ++tests_run;
}

void run_move_case(unsigned opcode, bool l, unsigned reg_field, unsigned rm_field,
                   const std::string& label) {
    const auto instruction = make_vex(opcode, l, reg_field, rm_field, 0, true);
    run_translation_case(instruction, label);
}

void run_zero_upper_case() {
    const auto instruction = make_zero_upper();
    const auto decoded = decode_one(instruction);
    const auto code = translate(instruction);
    VectorState actual{}, expected{};
    seed_state(actual);
    expected = actual;
    for (unsigned reg = 0; reg < kGuestVectorRegisters; ++reg)
        std::memset(&expected.ymm[reg][16], 0, 16);
    ExecutableCode executable(code);
    executable.run(actual);
    check_equal(actual, expected, "VZEROUPPER clears all upper halves and preserves all lower halves");
    ++tests_run;
    (void)decoded;
}

void expect_rejected(const std::vector<std::uint8_t>& bytes, const std::string& label) {
    bool rejected = false;
    try { (void)decode_one(bytes); }
    catch (const std::exception&) { rejected = true; }
    if (!rejected) fail("unsupported input was accepted: " + label);
    ++tests_run;
}

void write_binary(const std::string& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary);
    if (!out) fail("cannot create dump file: " + path);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!out) fail("failed writing dump file: " + path);
}

struct CpuFeatures {
    bool sse42{};
    bool avx{};
    bool avx2{};
    bool osxsave{};
};

CpuFeatures detect_cpu_features() {
    CpuFeatures f{};
#if defined(_MSC_VER) && defined(_M_X64)
    int regs[4]{};
    __cpuid(regs, 0);
    const unsigned max_leaf = static_cast<unsigned>(regs[0]);
    __cpuid(regs, 1);
    const unsigned ecx1 = static_cast<unsigned>(regs[2]);
    f.sse42 = (ecx1 & (1U << 20)) != 0;
    f.osxsave = (ecx1 & (1U << 27)) != 0;
    f.avx = (ecx1 & (1U << 28)) != 0;
    if (max_leaf >= 7) {
        __cpuidex(regs, 7, 0);
        f.avx2 = (static_cast<unsigned>(regs[1]) & (1U << 5)) != 0;
    }
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    unsigned eax = 0, ebx = 0, ecx = 0, edx = 0;
    const unsigned max_leaf = __get_cpuid_max(0, nullptr);
    if (max_leaf < 1 || !__get_cpuid(1, &eax, &ebx, &ecx, &edx))
        fail("CPUID leaf 1 is unavailable");
    f.sse42 = (ecx & bit_SSE4_2) != 0;
    f.osxsave = (ecx & bit_OSXSAVE) != 0;
    f.avx = (ecx & bit_AVX) != 0;
    if (max_leaf >= 7 && __get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx))
        f.avx2 = (ebx & bit_AVX2) != 0;
#else
    fail("CPUID validation is only implemented for x86-64 builds");
#endif
    return f;
}

} // namespace

int main(int argc, char** argv) {
    try {
        std::string dump_path;
        if (argc == 3 && std::string(argv[1]) == "--dump") dump_path = argv[2];
        else if (argc != 1) fail("usage: avx1_sse_poc [--dump generated_sse.bin]");

        const CpuFeatures cpu = detect_cpu_features();
        std::cout << "Guest CPUID: SSE4.2=" << (cpu.sse42 ? "yes" : "no")
                  << " AVX=" << (cpu.avx ? "yes" : "no")
                  << " AVX2=" << (cpu.avx2 ? "yes" : "no")
                  << " OSXSAVE=" << (cpu.osxsave ? "yes" : "no") << '\n';
        if (!cpu.sse42) fail("test target must expose SSE4.2");
        if (cpu.avx || cpu.avx2) fail("test target must hide AVX and AVX2");

        const std::array<Op, 7> ops = {Op::AddPS, Op::SubPS, Op::MulPS, Op::DivPS,
                                       Op::XorPS, Op::AndPS, Op::OrPS};
        std::vector<std::uint8_t> captured;
        for (Op op : ops) {
            const std::uint8_t opcode = opcode_for(op);
            run_translation_case(make_vex(opcode, false, 3, 7, 5), "VEX.128 op=" + std::to_string(opcode));
            run_translation_case(make_vex(opcode, true, 3, 7, 5), "VEX.256 op=" + std::to_string(opcode),
                                 (op == Op::AddPS ? &captured : nullptr));
            // Exercise VEX3 and YMM register numbers above 7.
            run_translation_case(make_vex(opcode, true, 12, 14, 9), "VEX3.256 high-register op=" + std::to_string(opcode));
        }

        // Literal bytes independently confirmed by GNU assembler/objdump fixtures.
        run_translation_case({0xC5, 0xF0, 0x58, 0xC2}, "assembler fixture: vaddps xmm0,xmm1,xmm2");
        run_translation_case({0xC4, 0x41, 0x34, 0x58, 0xE6}, "assembler fixture: vaddps ymm12,ymm9,ymm14");
        run_translation_case({0xC5, 0xF8, 0x10, 0xDF}, "assembler fixture: vmovups xmm3,xmm7");

        // Aliasing cases that commonly expose destructive-SSE lowering mistakes.
        run_translation_case(make_vex(0x58, true, 4, 6, 4), "VADDPS dst == src1");
        run_translation_case(make_vex(0x58, true, 4, 4, 6), "VADDPS dst == src2");
        run_translation_case(make_vex(0x58, true, 4, 6, 6), "VADDPS src1 == src2");

        // VMOVUPS/VMOVAPS register forms: both encodings have identical register data semantics.
        run_move_case(0x10, false, 3, 7, "VMOVUPS VEX.128 load-form register copy");
        run_move_case(0x10, true, 3, 7, "VMOVUPS VEX.256 load-form register copy");
        run_move_case(0x28, false, 3, 7, "VMOVAPS VEX.128 load-form register copy");
        run_move_case(0x28, true, 3, 7, "VMOVAPS VEX.256 load-form register copy");
        run_move_case(0x11, false, 3, 7, "VMOVUPS VEX.128 store-form register copy");
        run_move_case(0x11, true, 3, 7, "VMOVUPS VEX.256 store-form register copy");
        run_move_case(0x29, false, 3, 7, "VMOVAPS VEX.128 store-form register copy");
        run_move_case(0x29, true, 3, 7, "VMOVAPS VEX.256 store-form register copy");
        run_zero_upper_case();

        // Rejections are intentional: this POC must not silently claim AVX2 or memory-form coverage.
        expect_rejected({0xC5, 0xF0, 0xFA, 0xC2}, "unsupported VEX opcode");
        expect_rejected({0xC5, 0xF0, 0x58}, "truncated VEX instruction");
        expect_rejected({0xC5, 0xF0, 0x58, 0x02}, "memory operand");
        expect_rejected({0xC5, 0xFC, 0x77}, "VZEROALL is not VZEROUPPER");

        if (!dump_path.empty()) write_binary(dump_path, captured);
        std::cout << "PASS: " << tests_run << " checks; VEX instructions translated and executed as legacy SSE.\n";
        std::cout << "Scope: register-only AVX1 packed-single subset; 128/256-bit paths; 16 logical YMM registers.\n";
        std::cout << "No claim of full AVX1, AVX2, process-level DBT, Windows runtime, or game compatibility.\n";
        if (!dump_path.empty()) std::cout << "SSE code dump: " << dump_path << " (" << captured.size() << " bytes)\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
