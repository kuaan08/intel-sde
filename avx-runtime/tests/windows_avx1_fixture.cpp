#include <immintrin.h>
#include <cstdio>

extern "C" __m128 avx_fixture_add(__m128 left, __m128 right);

int main() {
    const __m128 left = _mm_set_ps(4.0f, 3.0f, 2.0f, 1.0f);
    const __m128 right = _mm_set_ps(8.0f, 7.0f, 6.0f, 5.0f);
    const __m128 result = avx_fixture_add(left, right);
    alignas(16) float values[4];
    _mm_store_ps(values, result);
    std::printf("AVX1 fixture: %.1f %.1f %.1f %.1f\n", values[0], values[1], values[2], values[3]);
    return values[0] == 6.0f && values[3] == 12.0f ? 0 : 1;
}
