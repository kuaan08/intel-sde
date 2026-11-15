#include <windows.h>
#include <cstdio>
#include <cstring>

using jit_function = void (*)(const float*, const float*, float*);

struct WorkerArgs {
    jit_function function = nullptr;
    float left = 0.0f;
    float right = 0.0f;
    float result = 0.0f;
};

static jit_function build_jit_function() {
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

static DWORD WINAPI worker(void* raw) {
    WorkerArgs* args = static_cast<WorkerArgs*>(raw);
    const float left[4] = {args->left, args->left, args->left, args->left};
    const float right[4] = {args->right, args->right, args->right, args->right};
    float result[4] = {};
    args->function(left, right, result);
    args->result = result[0];
    return args->result == args->left + args->right ? 0 : 1;
}

int main() {
    jit_function function = build_jit_function();
    if (!function) return 2;
    WorkerArgs args[4] = {{function, 1.0f, 2.0f}, {function, 10.0f, 4.0f},
                          {function, 100.0f, 8.0f}, {function, -5.0f, 3.0f}};
    HANDLE threads[4] = {};
    for (unsigned i = 0; i < 4; ++i) {
        threads[i] = CreateThread(nullptr, 0, worker, &args[i], 0, nullptr);
        if (!threads[i]) return 3;
    }
    WaitForMultipleObjects(4, threads, TRUE, INFINITE);
    DWORD statuses[4] = {};
    for (unsigned i = 0; i < 4; ++i) {
        GetExitCodeThread(threads[i], &statuses[i]);
        CloseHandle(threads[i]);
    }
    VirtualFree(reinterpret_cast<void*>(function), 0, MEM_RELEASE);
    for (unsigned i = 0; i < 4; ++i) {
        if (statuses[i] != 0) return 1;
    }
    std::printf("multithread JIT fixture: %.1f %.1f %.1f %.1f\n",
                args[0].result, args[1].result, args[2].result, args[3].result);
    return 0;
}
