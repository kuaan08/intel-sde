#include <cstdint>
#include <cstdio>
#include <windows.h>

#if defined(_MSC_VER)
#include <intrin.h>
static void query_cpuid(unsigned leaf, unsigned subleaf, unsigned* eax, unsigned* ebx,
                        unsigned* ecx, unsigned* edx) {
    int values[4] = {};
    __cpuidex(values, static_cast<int>(leaf), static_cast<int>(subleaf));
    *eax = static_cast<unsigned>(values[0]);
    *ebx = static_cast<unsigned>(values[1]);
    *ecx = static_cast<unsigned>(values[2]);
    *edx = static_cast<unsigned>(values[3]);
}
static std::uint64_t query_xcr0() { return _xgetbv(0); }
#else
#include <cpuid.h>
static void query_cpuid(unsigned leaf, unsigned subleaf, unsigned* eax, unsigned* ebx,
                        unsigned* ecx, unsigned* edx) {
    __cpuid_count(leaf, subleaf, *eax, *ebx, *ecx, *edx);
}
static std::uint64_t query_xcr0() {
    std::uint32_t eax = 0, edx = 0;
    __asm__ volatile(".byte 0x0f, 0x01, 0xd0" : "=a"(eax), "=d"(edx) : "c"(0));
    return static_cast<std::uint64_t>(eax) | (static_cast<std::uint64_t>(edx) << 32);
}
#endif

int main() {
    unsigned eax = 0, ebx = 0, ecx = 0, edx = 0;
    query_cpuid(0, 0, &eax, &ebx, &ecx, &edx);
    const unsigned max_leaf = eax;
    query_cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    const bool avx = (ecx & (1U << 28)) != 0;
    const bool osxsave = (ecx & (1U << 27)) != 0;
    const std::uint64_t xcr0 = query_xcr0();
    std::printf("CPUID fixture: max_leaf=%u avx=%u osxsave=%u xcr0=0x%llx\n",
                max_leaf, avx ? 1U : 0U, osxsave ? 1U : 0U,
                static_cast<unsigned long long>(xcr0));
    return avx && osxsave && xcr0 == 0x6 ? 0 : 1;
}
