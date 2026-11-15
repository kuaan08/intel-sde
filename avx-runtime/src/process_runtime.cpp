#include "avx_runtime/process_runtime.hpp"

#include <algorithm>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace avxlab {

std::size_t ProcessRuntimeTracker::refresh() {
#ifdef _WIN32
    std::size_t discovered = 0;
    std::uint8_t* cursor = nullptr;
    MEMORY_BASIC_INFORMATION information{};
    while (VirtualQuery(cursor, &information, sizeof(information)) == sizeof(information)) {
        const DWORD protection = information.Protect & 0xFFU;
        const bool executable = protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ ||
                                protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
        if (information.State == MEM_COMMIT && executable) {
            const auto base = reinterpret_cast<std::uint64_t>(information.BaseAddress);
            if (!executable_memory_.find(base)) executable_memory_.register_region(base, information.RegionSize);
            ++discovered;
        }
        const auto next = reinterpret_cast<std::uintptr_t>(information.BaseAddress) + information.RegionSize;
        if (next <= reinterpret_cast<std::uintptr_t>(cursor)) break;
        cursor = reinterpret_cast<std::uint8_t*>(next);
    }
    return discovered;
#else
    return 0;
#endif
}

void ProcessRuntimeTracker::register_executable_region(std::uint64_t base, std::size_t size) {
    executable_memory_.register_region(base, size);
}

void ProcessRuntimeTracker::unregister_executable_region(std::uint64_t base) {
    executable_memory_.unregister_region(base);
}

void ProcessRuntimeTracker::notify_code_write(std::uint64_t address, std::size_t size) {
    executable_memory_.invalidate_write(address, size);
}

std::uint64_t ProcessRuntimeTracker::generation_at(std::uint64_t address) const {
    return executable_memory_.generation_at(address);
}

void ProcessRuntimeTracker::add_diagnostic(UnsupportedInstructionReport report) {
    diagnostics_.push_back(std::move(report));
}

bool guest_feature_enabled(const Coverage& coverage, const std::string& feature) {
    const GuestFeatures features = derive_guest_features(coverage);
    if (feature == "sse4.2") return features.sse42;
    if (feature == "xsave") return features.xsave;
    if (feature == "osxsave") return features.osxsave;
    if (feature == "avx") return features.avx;
    if (feature == "avx2") return features.avx2;
    return false;
}

} // namespace avxlab
