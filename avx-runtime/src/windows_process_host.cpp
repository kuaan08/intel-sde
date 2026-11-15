#include "avx_runtime/windows_process_host.hpp"

#ifdef _WIN32
#include <windows.h>
#include <string>
#include <vector>

namespace avxlab {
namespace {

std::wstring widen(const std::string& value) {
    if (value.empty()) return std::wstring();
    const int length = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (length <= 0) return std::wstring();
    std::wstring result(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, &result[0], length);
    result.resize(static_cast<std::size_t>(length - 1));
    return result;
}

} // namespace

int run_process_with_shim(const std::string& target_path, const std::string& shim_path,
                          const std::string& target_arguments) {
    const std::wstring target = widen(target_path);
    const std::wstring shim = widen(shim_path);
    if (target.empty() || shim.empty()) return -1;

    std::wstring command = L"\"" + target + L"\"";
    if (!target_arguments.empty()) command += L" " + widen(target_arguments);
    std::vector<wchar_t> command_buffer(command.begin(), command.end());
    command_buffer.push_back(L'\0');
    STARTUPINFOW startup{}; startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr, command_buffer.data(), nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, nullptr, &startup, &process)) return -2;

    const SIZE_T bytes = (shim.size() + 1) * sizeof(wchar_t);
    void* remote_path = VirtualAllocEx(process.hProcess, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote_path || !WriteProcessMemory(process.hProcess, remote_path, shim.c_str(), bytes, nullptr)) {
        if (remote_path) VirtualFreeEx(process.hProcess, remote_path, 0, MEM_RELEASE);
        TerminateProcess(process.hProcess, 0xE001); CloseHandle(process.hThread); CloseHandle(process.hProcess); return -3;
    }
    HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
    FARPROC load_library = kernel ? GetProcAddress(kernel, "LoadLibraryW") : nullptr;
    HANDLE injector = load_library ? CreateRemoteThread(process.hProcess, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(load_library), remote_path, 0, nullptr) : nullptr;
    if (!injector) {
        VirtualFreeEx(process.hProcess, remote_path, 0, MEM_RELEASE); TerminateProcess(process.hProcess, 0xE002); CloseHandle(process.hThread); CloseHandle(process.hProcess); return -4;
    }
    WaitForSingleObject(injector, INFINITE);
    DWORD load_result = 0;
    const bool loaded = GetExitCodeThread(injector, &load_result) && load_result != 0;
    CloseHandle(injector);
    VirtualFreeEx(process.hProcess, remote_path, 0, MEM_RELEASE);
    if (!loaded) {
        TerminateProcess(process.hProcess, 0xE003);
        CloseHandle(process.hThread); CloseHandle(process.hProcess);
        return -5;
    }
    ResumeThread(process.hThread);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exit_code = 1; GetExitCodeProcess(process.hProcess, &exit_code);
    CloseHandle(process.hThread); CloseHandle(process.hProcess);
    return static_cast<int>(exit_code);
}

} // namespace avxlab
#else
namespace avxlab {
int run_process_with_shim(const std::string&, const std::string&, const std::string&) { return -100; }
} // namespace avxlab
#endif
