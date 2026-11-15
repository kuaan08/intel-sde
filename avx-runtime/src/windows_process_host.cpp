#include "avx_runtime/windows_process_host.hpp"

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <cstdint>
#include <cstring>
#include <iterator>
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

bool absolute_path(const std::wstring& input, std::wstring* output) {
    if (input.empty() || !output) return false;
    const DWORD required = GetFullPathNameW(input.c_str(), 0, nullptr, nullptr);
    if (required == 0) return false;
    std::wstring buffer(static_cast<std::size_t>(required), L'\0');
    const DWORD written = GetFullPathNameW(input.c_str(), required, &buffer[0], nullptr);
    if (written == 0 || written >= required) return false;
    buffer.resize(static_cast<std::size_t>(written));
    *output = buffer;
    return true;
}

std::size_t patch_feature_queries_in_module(HANDLE process, std::uintptr_t base,
                                            DWORD section_rva, DWORD section_size) {
    if (section_size == 0) return 0;
    std::vector<std::uint8_t> bytes(section_size);
    SIZE_T read = 0;
    if (!ReadProcessMemory(process, reinterpret_cast<const void*>(base + section_rva),
                           bytes.data(), bytes.size(), &read) || read != bytes.size()) return 0;
    std::size_t patched = 0;
    for (std::size_t i = 0; i + 2 < bytes.size(); ++i) {
        const bool cpuid = bytes[i] == 0x0F && bytes[i + 1] == 0xA2;
        const bool xgetbv = bytes[i] == 0x0F && bytes[i + 1] == 0x01 && bytes[i + 2] == 0xD0;
        if (!cpuid && !xgetbv) continue;
        BYTE trap = 0xCC;
        DWORD old_protection = 0;
        void* address = reinterpret_cast<void*>(base + section_rva + i);
        if (!VirtualProtectEx(process, address, 1, PAGE_EXECUTE_READWRITE, &old_protection)) continue;
        SIZE_T written = 0;
        const bool wrote = WriteProcessMemory(process, address, &trap, 1, &written) && written == 1;
        FlushInstructionCache(process, address, xgetbv ? 3 : 2);
        DWORD ignored = 0;
        VirtualProtectEx(process, address, 1, old_protection, &ignored);
        if (wrote) ++patched;
    }
    return patched;
}

std::size_t patch_main_module_feature_queries(HANDLE process, DWORD process_id) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    MODULEENTRY32W module{};
    module.dwSize = sizeof(module);
    if (!Module32FirstW(snapshot, &module)) {
        CloseHandle(snapshot);
        return 0;
    }
    const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(module.modBaseAddr);
    IMAGE_DOS_HEADER dos{};
    IMAGE_NT_HEADERS64 nt{};
    SIZE_T transferred = 0;
    if (!ReadProcessMemory(process, reinterpret_cast<const void*>(base), &dos, sizeof(dos), &transferred) ||
        dos.e_magic != IMAGE_DOS_SIGNATURE ||
        !ReadProcessMemory(process, reinterpret_cast<const void*>(base + dos.e_lfanew), &nt, sizeof(nt), &transferred) ||
        nt.Signature != IMAGE_NT_SIGNATURE || nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        CloseHandle(snapshot);
        return 0;
    }
    std::vector<IMAGE_SECTION_HEADER> sections(nt.FileHeader.NumberOfSections);
    const std::uintptr_t section_base = base + dos.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) +
                                        nt.FileHeader.SizeOfOptionalHeader;
    if (!sections.empty() && (!ReadProcessMemory(process, reinterpret_cast<const void*>(section_base),
                                                  sections.data(), sections.size() * sizeof(IMAGE_SECTION_HEADER),
                                                  &transferred) || transferred != sections.size() * sizeof(IMAGE_SECTION_HEADER))) {
        CloseHandle(snapshot);
        return 0;
    }
    std::size_t patched = 0;
    for (const IMAGE_SECTION_HEADER& section : sections) {
        if ((section.Characteristics & IMAGE_SCN_MEM_EXECUTE) == 0) continue;
        patched += patch_feature_queries_in_module(process, base, section.VirtualAddress,
                                                   section.Misc.VirtualSize);
    }
    CloseHandle(snapshot);
    return patched;
}

bool experimental_feature_patch_enabled() {
    wchar_t value[8] = {};
    const DWORD length = GetEnvironmentVariableW(L"SDE_AVX_LAB_EXPERIMENTAL_CPUID", value,
                                                  static_cast<DWORD>(std::size(value)));
    return length > 0 && value[0] == L'1';
}

} // namespace

int run_process_with_shim(const std::string& target_path, const std::string& shim_path,
                          const std::string& target_arguments) {
    const std::wstring target_input = widen(target_path);
    const std::wstring shim_input = widen(shim_path);
    std::wstring target;
    std::wstring shim;
    if (!absolute_path(target_input, &target)) return -11;
    if (!absolute_path(shim_input, &shim)) return -12;
    if (GetFileAttributesW(target.c_str()) == INVALID_FILE_ATTRIBUTES) return -13;
    if (GetFileAttributesW(shim.c_str()) == INVALID_FILE_ATTRIBUTES) return -14;

    std::wstring command = L"\"" + target + L"\"";
    if (!target_arguments.empty()) command += L" " + widen(target_arguments);
    std::vector<wchar_t> command_buffer(command.begin(), command.end());
    command_buffer.push_back(L'\0');
    STARTUPINFOW startup{}; startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(target.c_str(), command_buffer.data(), nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, nullptr, &startup, &process)) return -2;

    const SIZE_T bytes = (shim.size() + 1) * sizeof(wchar_t);
    void* remote_path = VirtualAllocEx(process.hProcess, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote_path || !WriteProcessMemory(process.hProcess, remote_path, shim.c_str(), bytes, nullptr)) {
        if (remote_path) VirtualFreeEx(process.hProcess, remote_path, 0, MEM_RELEASE);
        TerminateProcess(process.hProcess, 0xE001); CloseHandle(process.hThread); CloseHandle(process.hProcess); return -3;
    }
    HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
    FARPROC load_library = kernel ? GetProcAddress(kernel, "LoadLibraryW") : nullptr;
    LPTHREAD_START_ROUTINE load_library_thread = nullptr;
    if (load_library) {
        static_assert(sizeof(load_library_thread) == sizeof(load_library), "Windows function pointer sizes differ");
        std::memcpy(&load_library_thread, &load_library, sizeof(load_library_thread));
    }
    HANDLE injector = load_library_thread ? CreateRemoteThread(process.hProcess, nullptr, 0,
                                                                load_library_thread, remote_path, 0, nullptr) : nullptr;
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
    // Experimental feature-query virtualization is opt-in until native Windows
    // validation covers loader and JIT modules. The shim handles INT3 followed by
    // CPUID/XGETBV in the main image only.
    if (experimental_feature_patch_enabled()) {
        (void)patch_main_module_feature_queries(process.hProcess, process.dwProcessId);
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
