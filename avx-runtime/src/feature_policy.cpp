#include "avx_runtime/feature_policy.hpp"

#include <sstream>

namespace avxlab {

GuestFeatures derive_guest_features(const Coverage& coverage) {
    GuestFeatures features;
    features.avx = coverage.avx1_register && coverage.avx1_memory;
    features.avx2 = features.avx && coverage.avx2;
    features.xsave = features.avx;
    features.osxsave = features.avx;
    return features;
}

std::string feature_report(const GuestFeatures& features) {
    std::ostringstream out;
    out << "SSE4.2=" << (features.sse42 ? "yes" : "no")
        << " XSAVE=" << (features.xsave ? "yes" : "no")
        << " OSXSAVE=" << (features.osxsave ? "yes" : "no")
        << " AVX=" << (features.avx ? "yes" : "no")
        << " AVX2=" << (features.avx2 ? "yes" : "no");
    return out.str();
}

VirtualCpuidResult virtual_cpuid(const Coverage& coverage, std::uint32_t leaf, std::uint32_t subleaf) {
    const GuestFeatures features = derive_guest_features(coverage);
    VirtualCpuidResult result;
    if (leaf == 0) { result.eax = 7; return result; }
    if (leaf == 1 && subleaf == 0) {
        result.ecx = (1U << 20); // SSE4.2
        if (features.xsave) result.ecx |= 1U << 26;
        if (features.osxsave) result.ecx |= 1U << 27;
        if (features.avx) result.ecx |= 1U << 28;
        return result;
    }
    if (leaf == 7 && subleaf == 0) {
        if (features.avx2) result.ebx |= 1U << 5;
        return result;
    }
    return result;
}

bool virtual_xgetbv(const Coverage& coverage, std::uint32_t index, std::uint64_t* value) {
    if (!value || index != 0) return false;
    const GuestFeatures features = derive_guest_features(coverage);
    if (!features.osxsave) return false;
    *value = features.avx ? 0x6ULL : 0x1ULL;
    return true;
}

} // namespace avxlab
