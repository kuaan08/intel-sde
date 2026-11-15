#pragma once

#include <cstdint>
#include <string>

namespace avxlab {

struct Coverage {
    bool avx1_register = true;
    bool avx1_memory = false;
    bool avx2 = false;
};

struct GuestFeatures {
    bool sse42 = true;
    bool xsave = false;
    bool osxsave = false;
    bool avx = false;
    bool avx2 = false;
};

struct VirtualCpuidResult {
    std::uint32_t eax = 0;
    std::uint32_t ebx = 0;
    std::uint32_t ecx = 0;
    std::uint32_t edx = 0;
};

GuestFeatures derive_guest_features(const Coverage& coverage);
std::string feature_report(const GuestFeatures& features);
VirtualCpuidResult virtual_cpuid(const Coverage& coverage, std::uint32_t leaf, std::uint32_t subleaf = 0);
bool virtual_xgetbv(const Coverage& coverage, std::uint32_t index, std::uint64_t* value);

} // namespace avxlab
