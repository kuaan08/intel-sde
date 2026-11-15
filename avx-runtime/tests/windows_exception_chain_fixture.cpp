#include <windows.h>
#include <cstdio>
#include <cstring>

static volatile LONG g_handled = 0;

static LONG CALLBACK fallback_handler(PEXCEPTION_POINTERS pointers) {
    if (!pointers || !pointers->ExceptionRecord || !pointers->ContextRecord) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    if (pointers->ExceptionRecord->ExceptionCode != EXCEPTION_ILLEGAL_INSTRUCTION) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    // The fixture's intentionally unsupported VEX instruction is exactly four
    // bytes long. The shim must decline it and let this later VEH recover.
    pointers->ContextRecord->Rip += 4;
    InterlockedExchange(&g_handled, 1);
    return EXCEPTION_CONTINUE_EXECUTION;
}

using fixture_function = int (*)();

int main() {
    static const unsigned char code[] = {
        0xC5, 0xF0, 0xFA, 0xC2, // intentionally unsupported VEX form
        0x31, 0xC0,             // xor eax,eax
        0xC3,                   // ret
    };
    void* executable = VirtualAlloc(nullptr, sizeof(code), MEM_COMMIT | MEM_RESERVE,
                                     PAGE_EXECUTE_READWRITE);
    if (!executable) return 2;
    std::memcpy(executable, code, sizeof(code));
    FlushInstructionCache(GetCurrentProcess(), executable, sizeof(code));
    PVOID handler = AddVectoredExceptionHandler(0, fallback_handler);
    if (!handler) return 3;
    const int result = reinterpret_cast<fixture_function>(executable)();
    RemoveVectoredExceptionHandler(handler);
    VirtualFree(executable, 0, MEM_RELEASE);
    std::printf("exception-chain fixture: result=%d handled=%ld\n", result, g_handled);
    return result == 0 && g_handled != 0 ? 0 : 1;
}
