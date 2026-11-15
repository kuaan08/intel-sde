#ifdef _WIN32
#include <windows.h>
#if defined(__GNUC__)
#include <cpuid.h>
#else
#include <intrin.h>
#endif
#include <array>
#include <cstdint>
#include <cstring>

namespace {

M128A* xmm_register(CONTEXT* context, unsigned index) {
    M128A* registers[] = {&context->Xmm0, &context->Xmm1, &context->Xmm2, &context->Xmm3,
        &context->Xmm4, &context->Xmm5, &context->Xmm6, &context->Xmm7, &context->Xmm8,
        &context->Xmm9, &context->Xmm10, &context->Xmm11, &context->Xmm12, &context->Xmm13,
        &context->Xmm14, &context->Xmm15};
    return index < 16 ? registers[index] : nullptr;
}

std::uint64_t* gpr_register(CONTEXT* context, unsigned index) {
    std::uint64_t* registers[] = {&context->Rax, &context->Rcx, &context->Rdx, &context->Rbx,
        &context->Rsp, &context->Rbp, &context->Rsi, &context->Rdi, &context->R8,
        &context->R9, &context->R10, &context->R11, &context->R12, &context->R13,
        &context->R14, &context->R15};
    return index < 16 ? registers[index] : nullptr;
}

bool safe_read(const void* source, void* destination, std::size_t bytes) {
#ifdef _MSC_VER
    __try {
        std::memcpy(destination, source, bytes);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
#else
    std::memcpy(destination, source, bytes);
    return true;
#endif
}

bool safe_write(void* destination, const void* source, std::size_t bytes) {
#ifdef _MSC_VER
    __try {
        std::memcpy(destination, source, bytes);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
#else
    std::memcpy(destination, source, bytes);
    return true;
#endif
}

bool emulate_virtual_feature_query(CONTEXT* context) {
    if (!context || context->Rip == 0) return false;
    std::uint8_t opcode[4] = {};
    std::uintptr_t instruction_address = context->Rip;
    unsigned trap_bytes_before_rip = 0;
    if (!safe_read(reinterpret_cast<const void*>(instruction_address), opcode, sizeof(opcode))) return false;
    if (opcode[0] == 0xCC) {
        trap_bytes_before_rip = 1;
        std::memmove(opcode, opcode + 1, 3);
    } else {
        std::uint8_t previous = 0;
        if (instruction_address == 0 ||
            !safe_read(reinterpret_cast<const void*>(instruction_address - 1), &previous, 1) ||
            previous != 0xCC) return false;
        instruction_address -= 1;
        trap_bytes_before_rip = 1;
        if (!safe_read(reinterpret_cast<const void*>(instruction_address), opcode, sizeof(opcode))) return false;
        std::memmove(opcode, opcode + 1, 3);
    }
    // The patcher replaces the first byte (0x0F) with INT3. The bytes visible
    // after the trap are therefore A2 for CPUID and 01 D0 for XGETBV.
    if (trap_bytes_before_rip != 0 && opcode[0] == 0xA2) {
        unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
#if defined(__GNUC__)
        __cpuid_count(static_cast<unsigned int>(context->Rax),
                      static_cast<unsigned int>(context->Rcx), eax, ebx, ecx, edx);
#else
        int values[4] = {};
        __cpuidex(values, static_cast<int>(context->Rax), static_cast<int>(context->Rcx));
        eax = static_cast<unsigned int>(values[0]); ebx = static_cast<unsigned int>(values[1]);
        ecx = static_cast<unsigned int>(values[2]); edx = static_cast<unsigned int>(values[3]);
#endif
        if (static_cast<unsigned int>(context->Rax) == 1) {
            ecx |= (1U << 26) | (1U << 27) | (1U << 28); // XSAVE, OSXSAVE, AVX
        }
        if (static_cast<unsigned int>(context->Rax) == 7 &&
            static_cast<unsigned int>(context->Rcx) == 0) {
            ebx &= ~(1U << 5); // AVX2 remains hidden until its backend is complete.
        }
        context->Rax = eax; context->Rbx = ebx; context->Rcx = ecx; context->Rdx = edx;
        context->Rip = instruction_address + 2;
        return true;
    }
    if (trap_bytes_before_rip != 0 && opcode[0] == 0x01 && opcode[1] == 0xD0 && context->Rcx == 0) {
        context->Rax = 6; // XMM and YMM state enabled in the virtual guest view.
        context->Rdx = 0;
        context->Rip = instruction_address + 3;
        return true;
    }
    return false;
}

bool decode_memory_address(CONTEXT* context, const std::uint8_t* ip, unsigned vex_length,
                           std::uint8_t modrm, unsigned vex_b, unsigned vex_x,
                           std::uint64_t* address, unsigned* instruction_length) {
    const unsigned mod = modrm >> 6;
    const unsigned rm_field = modrm & 7U;
    if (mod == 3) return false;

    unsigned cursor = vex_length + 2; // VEX prefix + opcode + ModRM
    std::uint64_t base = 0;
    bool has_base = true;
    unsigned base_reg = 0;

    if (rm_field == 4) {
        if (cursor >= 15) return false;
        const std::uint8_t sib = ip[cursor++];
        const unsigned scale = 1U << (sib >> 6);
        const unsigned index_field = (sib >> 3) & 7U;
        const unsigned base_field = sib & 7U;
        std::uint64_t index_value = 0;
        if (index_field != 4) {
            std::uint64_t* index = gpr_register(context, index_field | (vex_x << 3));
            if (!index) return false;
            index_value = (*index) * scale;
        }
        if (mod == 0 && base_field == 5) {
            has_base = false;
        } else {
            base_reg = base_field | (vex_b << 3);
            std::uint64_t* base_pointer = gpr_register(context, base_reg);
            if (!base_pointer) return false;
            base = *base_pointer;
        }
        base += index_value;
    } else if (mod == 0 && rm_field == 5) {
        has_base = false;
        base = context->Rip + cursor + 4;
    } else {
        base_reg = rm_field | (vex_b << 3);
        std::uint64_t* base_pointer = gpr_register(context, base_reg);
        if (!base_pointer) return false;
        base = *base_pointer;
    }

    std::int64_t displacement = 0;
    if (mod == 1) {
        displacement = static_cast<std::int8_t>(ip[cursor++]);
    } else if (mod == 2 || (mod == 0 && !has_base)) {
        std::int32_t value = 0;
        std::memcpy(&value, ip + cursor, sizeof(value));
        cursor += sizeof(value);
        displacement = value;
    }
    *address = static_cast<std::uint64_t>(static_cast<std::int64_t>(base) + displacement);
    *instruction_length = cursor;
    return true;
}

bool emulate_vex128(CONTEXT* context) {
    const auto* ip = reinterpret_cast<const std::uint8_t*>(context->Rip);
    if (!ip) return false;

    unsigned vex_length = 0;
    unsigned vex_b = 0;
    unsigned vex_x = 0;
    unsigned src1 = 0;
    unsigned vector_length = 0;
    unsigned mandatory_prefix = 0;
    unsigned opcode_offset = 0;
    if (ip[0] == 0xC5) {
        const std::uint8_t prefix = ip[1];
        if ((prefix & 0x04U) != 0 || ((prefix & 0x03U) != 0 && (prefix & 0x03U) != 2)) return false;
        src1 = ((~(prefix >> 3)) & 15U);
        vector_length = (prefix >> 2) & 1U;
        mandatory_prefix = prefix & 3U;
        vex_length = 2;
        opcode_offset = 2;
    } else if (ip[0] == 0xC4) {
        const std::uint8_t p0 = ip[1];
        const std::uint8_t p1 = ip[2];
        if ((p0 & 0x1FU) != 1 || (p1 & 0x80U) != 0 ||
            ((p1 & 0x03U) != 0 && (p1 & 0x03U) != 2)) return false;
        vex_b = ((p0 >> 5) & 1U) ^ 1U;
        vex_x = ((p0 >> 6) & 1U) ^ 1U;
        src1 = ((~(p1 >> 3)) & 15U);
        vector_length = (p1 >> 2) & 1U;
        mandatory_prefix = p1 & 3U;
        vex_length = 3;
        opcode_offset = 3;
    } else {
        return false;
    }

    if (vector_length != 0) return false; // VEX.256 requires YMM state not exposed by CONTEXT.
    const std::uint8_t opcode = ip[opcode_offset];
    if (opcode == 0x77) {
        if (ip[0] != 0xC5 || src1 != 15) return false;
        context->Rip += vex_length + 1;
        return true;
    }

    const std::uint8_t modrm = ip[opcode_offset + 1];
    const unsigned mod = modrm >> 6;
    const unsigned dst = ((((ip[0] == 0xC5 ? ip[1] : ip[1]) >> 7) & 1U) ^ 1U) * 8U |
                         ((modrm >> 3) & 7U);
    const unsigned rm = (modrm & 7U) | (vex_b << 3);
    if (dst >= 16 || src1 >= 16) return false;

    const bool scalar_single = mandatory_prefix == 2 &&
        (opcode == 0x10 || opcode == 0x11 || opcode == 0x58 || opcode == 0x5C ||
         opcode == 0x59 || opcode == 0x5E);

    M128A* destination = xmm_register(context, dst);
    M128A* first = xmm_register(context, src1);
    if (!destination || !first) return false;

    std::array<std::uint8_t, 16> left{};
    std::array<std::uint8_t, 16> right{};
    std::array<std::uint8_t, 16> result{};
    if (!safe_read(first, left.data(), left.size())) return false;

    unsigned instruction_length = vex_length + 2;
    const unsigned operand_bytes = scalar_single ? 4U : 16U;
    if (mod == 3) {
        if (rm >= 16) return false;
        M128A* second = xmm_register(context, rm);
        if (!second || !safe_read(second, right.data(), operand_bytes)) return false;
    } else {
        std::uint64_t address = 0;
        if (!decode_memory_address(context, ip, vex_length, modrm, vex_b, vex_x,
                                   &address, &instruction_length) ||
            !safe_read(reinterpret_cast<const void*>(address), right.data(), operand_bytes)) {
            return false;
        }
    }

    if (scalar_single) {
        if (opcode == 0x11) {
            if (mod == 3) return false;
            std::uint64_t address = 0;
            if (!decode_memory_address(context, ip, vex_length, modrm, vex_b, vex_x,
                                       &address, &instruction_length) ||
                !safe_write(reinterpret_cast<void*>(address), destination, 4)) return false;
            context->Rip += instruction_length;
            return true;
        }
        if (opcode == 0x10) {
            std::array<std::uint8_t, 16> combined = left;
            std::memcpy(combined.data(), right.data(), 4);
            if (!safe_write(destination, combined.data(), combined.size())) return false;
            context->Rip += instruction_length;
            return true;
        }
        float left_value = 0.0f;
        float right_value = 0.0f;
        float value = 0.0f;
        std::memcpy(&left_value, left.data(), 4);
        std::memcpy(&right_value, right.data(), 4);
        switch (opcode) {
        case 0x58: value = left_value + right_value; break;
        case 0x5C: value = left_value - right_value; break;
        case 0x59: value = left_value * right_value; break;
        case 0x5E: value = left_value / right_value; break;
        default: return false;
        }
        std::array<std::uint8_t, 16> combined = left;
        std::memcpy(combined.data(), &value, 4);
        if (!safe_write(destination, combined.data(), combined.size())) return false;
        context->Rip += instruction_length;
        return true;
    }

    if (opcode == 0x10 || opcode == 0x28) {
        if (!safe_write(destination, right.data(), right.size())) return false;
        context->Rip += instruction_length;
        return true;
    }
    if (opcode == 0x11 || opcode == 0x29) {
        if (mod == 3) {
            M128A* second = xmm_register(context, rm);
            if (!second || !safe_write(second, destination, right.size())) return false;
        } else {
            std::uint64_t address = 0;
            if (!decode_memory_address(context, ip, vex_length, modrm, vex_b, vex_x,
                                       &address, &instruction_length) ||
                !safe_write(reinterpret_cast<void*>(address), destination, right.size())) {
                return false;
            }
        }
        context->Rip += instruction_length;
        return true;
    }

    for (unsigned lane = 0; lane < 4; ++lane) {
        float left_value = 0.0f;
        float right_value = 0.0f;
        float value = 0.0f;
        std::memcpy(&left_value, left.data() + lane * 4, 4);
        std::memcpy(&right_value, right.data() + lane * 4, 4);
        switch (opcode) {
        case 0x58: value = left_value + right_value; break;
        case 0x5C: value = left_value - right_value; break;
        case 0x59: value = left_value * right_value; break;
        case 0x5E: value = left_value / right_value; break;
        default: {
            std::uint32_t x = 0, y = 0;
            std::memcpy(&x, left.data() + lane * 4, 4);
            std::memcpy(&y, right.data() + lane * 4, 4);
            if (opcode == 0x57) x = 0;
            else if (opcode == 0x54) x &= y;
            else if (opcode == 0x56) x |= y;
            else if (opcode == 0x55) x = (~x) & y;
            else if (opcode == 0xDB) x &= y;
            else if (opcode == 0xEB) x |= y;
            else if (opcode == 0xEF) x ^= y;
            else return false;
            std::memcpy(&value, &x, 4);
            break;
        }
        }
        std::memcpy(result.data() + lane * 4, &value, 4);
    }
    if (!safe_write(destination, result.data(), result.size())) return false;
    context->Rip += instruction_length;
    return true;
}

LONG CALLBACK sde_avx_vectored_handler(PEXCEPTION_POINTERS pointers) {
    if (!pointers || !pointers->ExceptionRecord || !pointers->ContextRecord) return EXCEPTION_CONTINUE_SEARCH;
    if (pointers->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT &&
        emulate_virtual_feature_query(pointers->ContextRecord)) return EXCEPTION_CONTINUE_EXECUTION;
    if (pointers->ExceptionRecord->ExceptionCode != EXCEPTION_ILLEGAL_INSTRUCTION) return EXCEPTION_CONTINUE_SEARCH;
    return emulate_vex128(pointers->ContextRecord) ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}

PVOID g_handler = nullptr;

} // namespace

extern "C" __declspec(dllexport) const char* SdeAvxShimVersion() { return "SDE AVX Lab Windows shim 0.3; VEX128 subset with opt-in CPUID/XGETBV virtualization"; }

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) { DisableThreadLibraryCalls(instance); g_handler = AddVectoredExceptionHandler(1, sde_avx_vectored_handler); }
    else if (reason == DLL_PROCESS_DETACH && g_handler) { RemoveVectoredExceptionHandler(g_handler); g_handler = nullptr; }
    return TRUE;
}
#endif
