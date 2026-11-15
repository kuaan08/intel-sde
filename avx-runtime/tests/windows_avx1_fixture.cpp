#include <immintrin.h>
#include <cstdio>
#include <cstring>
#include <windows.h>

using avx_fixture_function = __m128 (*)(__m128, __m128);

static avx_fixture_function build_vex128_add() {
    // vaddps xmm0, xmm0, xmm1; vzeroupper; ret
    static const unsigned char code[] = {
        0xC5, 0xF8, 0x58, 0xC1,
        0xC5, 0xF8, 0x77,
        0xC3,
    };
    void* executable = VirtualAlloc(nullptr, sizeof(code), MEM_COMMIT | MEM_RESERVE,
                                     PAGE_EXECUTE_READWRITE);
    if (!executable) {
        return nullptr;
    }
    std::memcpy(executable, code, sizeof(code));
    FlushInstructionCache(GetCurrentProcess(), executable, sizeof(code));
    return reinterpret_cast<avx_fixture_function>(executable);
}

int main() {
    const avx_fixture_function avx_fixture_add = build_vex128_add();
    if (!avx_fixture_add) {
        std::printf("AVX1 fixture: VirtualAlloc failed\n");
        return 2;
    }
    const __m128 left = _mm_set_ps(4.0f, 3.0f, 2.0f, 1.0f);
    const __m128 right = _mm_set_ps(8.0f, 7.0f, 6.0f, 5.0f);
    const __m128 result = avx_fixture_add(left, right);
    alignas(16) float values[4];
    _mm_store_ps(values, result);
    std::printf("AVX1 fixture: %.1f %.1f %.1f %.1f\n", values[0], values[1], values[2], values[3]);
    return values[0] == 6.0f && values[3] == 12.0f ? 0 : 1;
}
