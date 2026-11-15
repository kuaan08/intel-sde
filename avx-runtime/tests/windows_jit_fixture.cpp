#include <cstdio>
#include <cstring>
#include <windows.h>

using jit_function = void (*)(const float*, const float*, float*);

static jit_function build_jit_function() {
    // Legacy loads, VEX.128 VADDPS, legacy store, VZEROUPPER, RET.
    // The bytes are emitted after main starts to model a small JIT code page.
    static const unsigned char code[] = {
        0x0F, 0x10, 0x01,
        0x0F, 0x10, 0x0A,
        0xC5, 0xF8, 0x58, 0xC1,
        0x41, 0x0F, 0x11, 0x00,
        0xC5, 0xF8, 0x77,
        0xC3,
    };
    void* executable = VirtualAlloc(nullptr, sizeof(code), MEM_COMMIT | MEM_RESERVE,
                                     PAGE_READWRITE);
    if (!executable) return nullptr;
    std::memcpy(executable, code, sizeof(code));
    DWORD old_protection = 0;
    if (!VirtualProtect(executable, sizeof(code), PAGE_EXECUTE_READ, &old_protection)) {
        VirtualFree(executable, 0, MEM_RELEASE);
        return nullptr;
    }
    FlushInstructionCache(GetCurrentProcess(), executable, sizeof(code));
    return reinterpret_cast<jit_function>(executable);
}

int main() {
    const float left[4] = {10.0f, 20.0f, 30.0f, 40.0f};
    const float right[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float result[4] = {};
    jit_function function = build_jit_function();
    if (!function) {
        std::printf("JIT fixture: executable allocation failed\n");
        return 2;
    }
    function(left, right, result);
    std::printf("JIT fixture: %.1f %.1f %.1f %.1f\n",
                result[0], result[1], result[2], result[3]);
    VirtualFree(reinterpret_cast<void*>(function), 0, MEM_RELEASE);
    return result[0] == 11.0f && result[1] == 22.0f &&
                   result[2] == 33.0f && result[3] == 44.0f ? 0 : 1;
}
