#include <cstdio>
#include <cstring>
#include <windows.h>

using avx_fixture_function = void (*)(const float*, const float*, float*);

static avx_fixture_function build_vex128_add() {
    // movups xmm0, [rcx]; movups xmm1, [rdx]; vaddps xmm0, xmm0, xmm1;
    // movups [r8], xmm0; vzeroupper; ret
    static const unsigned char code[] = {
        0x0F, 0x10, 0x01,
        0x0F, 0x10, 0x0A,
        0xC5, 0xF8, 0x58, 0xC1,
        0x41, 0x0F, 0x11, 0x00,
        0xC5, 0xF8, 0x77,
        0xC3,
    };
    void* executable = VirtualAlloc(nullptr, sizeof(code), MEM_COMMIT | MEM_RESERVE,
                                     PAGE_EXECUTE_READWRITE);
    if (!executable) return nullptr;
    std::memcpy(executable, code, sizeof(code));
    FlushInstructionCache(GetCurrentProcess(), executable, sizeof(code));
    return reinterpret_cast<avx_fixture_function>(executable);
}

static avx_fixture_function build_vex_scalar_add() {
    // movups xmm0, [rcx]; movups xmm1, [rdx]; vaddss xmm0, xmm0, xmm1;
    // movups [r8], xmm0; vzeroupper; ret
    static const unsigned char code[] = {
        0x0F, 0x10, 0x01,
        0x0F, 0x10, 0x0A,
        0xC5, 0xFA, 0x58, 0xC1,
        0x41, 0x0F, 0x11, 0x00,
        0xC5, 0xF8, 0x77,
        0xC3,
    };
    void* executable = VirtualAlloc(nullptr, sizeof(code), MEM_COMMIT | MEM_RESERVE,
                                     PAGE_EXECUTE_READWRITE);
    if (!executable) return nullptr;
    std::memcpy(executable, code, sizeof(code));
    FlushInstructionCache(GetCurrentProcess(), executable, sizeof(code));
    return reinterpret_cast<avx_fixture_function>(executable);
}

int main() {
    const float left[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    const float right[4] = {5.0f, 6.0f, 7.0f, 8.0f};
    float values[4] = {};

    const avx_fixture_function avx_fixture_add = build_vex128_add();
    if (!avx_fixture_add) {
        std::printf("AVX1 fixture: VirtualAlloc failed\n");
        return 2;
    }
    avx_fixture_add(left, right, values);
    std::printf("AVX1 fixture: %.1f %.1f %.1f %.1f\n", values[0], values[1], values[2], values[3]);
    if (values[0] != 6.0f || values[3] != 12.0f) return 1;

    const avx_fixture_function avx_fixture_scalar = build_vex_scalar_add();
    if (!avx_fixture_scalar) return 2;
    std::memset(values, 0, sizeof(values));
    avx_fixture_scalar(left, right, values);
    std::printf("AVX1 scalar fixture: %.1f %.1f %.1f %.1f\n", values[0], values[1], values[2], values[3]);
    return values[0] == 6.0f && values[1] == 2.0f && values[3] == 4.0f ? 0 : 1;
}
