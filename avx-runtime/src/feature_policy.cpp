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

} // namespace avxlab
