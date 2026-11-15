// Guest-side AVX1 fixture. It is compiled for coverage validation and is not
// executed on the no-AVX CI host. The independent process runtime will use
// this fixture as a future Windows application test target.
#include <immintrin.h>
#include <cstdio>

int main() {
    const __m128 a = _mm_set_ps(4.0f, 3.0f, 2.0f, 1.0f);
    const __m128 b = _mm_set_ps(8.0f, 7.0f, 6.0f, 5.0f);
    const __m128 c = _mm_add_ps(a, b);
    alignas(16) float result[4];
    _mm_store_ps(result, c);
    std::printf("AVX1 fixture: %.1f %.1f %.1f %.1f\n", result[0], result[1], result[2], result[3]);
    return result[0] == 6.0f && result[3] == 12.0f ? 0 : 1;
}
