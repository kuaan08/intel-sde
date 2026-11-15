#ifdef _WIN32
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>

namespace {

M128* xmm_register(CONTEXT* context, unsigned index) {
    M128* registers[] = {&context->Xmm0, &context->Xmm1, &context->Xmm2, &context->Xmm3, &context->Xmm4, &context->Xmm5, &context->Xmm6, &context->Xmm7, &context->Xmm8, &context->Xmm9, &context->Xmm10, &context->Xmm11, &context->Xmm12, &context->Xmm13, &context->Xmm14, &context->Xmm15};
    return index < 16 ? registers[index] : nullptr;
}

bool emulate_vex128(CONTEXT* context) {
    const auto* ip = reinterpret_cast<const std::uint8_t*>(context->Rip);
    if (!ip || ip[0] != 0xC5 || (ip[1] & 0x04U) != 0 || (ip[1] & 0x03U) != 0) return false;
    const std::uint8_t prefix = ip[1];
    const unsigned dst = (((prefix >> 7) & 1U) ^ 1U) * 8U | ((ip[3] >> 3) & 7U);
    const unsigned src1 = (~(prefix >> 3)) & 15U;
    const unsigned src2 = ip[3] & 7U;
    const unsigned mod = ip[3] >> 6;
    const std::uint8_t opcode = ip[2];
    if (opcode == 0x77) { context->Rip += 3; return true; }
    if (mod != 3 || dst >= 16 || src1 >= 16 || src2 >= 8) return false;
    M128* destination = xmm_register(context, dst);
    M128* first = xmm_register(context, src1);
    M128* second = xmm_register(context, src2);
    if (!destination || !first || !second) return false;
    std::array<std::uint8_t, 16> a{}, b{}, result{};
    std::memcpy(a.data(), first, 16); std::memcpy(b.data(), second, 16);
    if (opcode == 0x10 || opcode == 0x28) { std::memcpy(destination, b.data(), 16); context->Rip += 4; return true; }
    if (opcode == 0x11 || opcode == 0x29) { std::memcpy(second, destination, 16); context->Rip += 4; return true; }
    for (unsigned lane = 0; lane < 4; ++lane) {
        float left = 0.0f, right = 0.0f, value = 0.0f;
        std::memcpy(&left, a.data() + lane * 4, 4); std::memcpy(&right, b.data() + lane * 4, 4);
        switch (opcode) { case 0x58: value = left + right; break; case 0x5C: value = left - right; break; case 0x59: value = left * right; break; case 0x5E: value = left / right; break; default: std::memcpy(&value, a.data() + lane * 4, 4); if (opcode == 0x57) value = 0.0f; else { std::uint32_t x = 0, y = 0; std::memcpy(&x, a.data() + lane * 4, 4); std::memcpy(&y, b.data() + lane * 4, 4); x = opcode == 0x54 ? (x & y) : opcode == 0x56 ? (x | y) : (x ^ y); std::memcpy(&value, &x, 4); } break; }
        std::memcpy(result.data() + lane * 4, &value, 4);
    }
    std::memcpy(destination, result.data(), 16); context->Rip += 4; return true;
}

LONG CALLBACK sde_avx_vectored_handler(PEXCEPTION_POINTERS pointers) {
    if (!pointers || !pointers->ExceptionRecord || pointers->ExceptionRecord->ExceptionCode != EXCEPTION_ILLEGAL_INSTRUCTION) return EXCEPTION_CONTINUE_SEARCH;
    return emulate_vex128(pointers->ContextRecord) ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}

PVOID g_handler = nullptr;

} // namespace

extern "C" __declspec(dllexport) const char* SdeAvxShimVersion() { return "SDE AVX Lab Windows shim 0.1; VEX128 register-only VEH"; }

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) { DisableThreadLibraryCalls(instance); g_handler = AddVectoredExceptionHandler(1, sde_avx_vectored_handler); }
    else if (reason == DLL_PROCESS_DETACH && g_handler) { RemoveVectoredExceptionHandler(g_handler); g_handler = nullptr; }
    return TRUE;
}
#endif
