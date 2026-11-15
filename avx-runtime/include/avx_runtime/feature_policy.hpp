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

GuestFeatures derive_guest_features(const Coverage& coverage);
std::string feature_report(const GuestFeatures& features);

} // namespace avxlab
