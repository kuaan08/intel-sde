#include "avx_runtime/executable_memory.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace avxlab {

bool ExecutableRegion::overlaps(std::uint64_t address, std::size_t length) const {
    if (size == 0 || length == 0) return false;
    const std::uint64_t max = std::numeric_limits<std::uint64_t>::max();
    const std::uint64_t end = address > max - length ? max : address + length;
    const std::uint64_t region_end = base > max - size ? max : base + size;
    return address < region_end && base < end;
}

void ExecutableMemoryMap::register_region(std::uint64_t base, std::size_t size) {
    if (size == 0) throw std::invalid_argument("executable region size must be non-zero");
    unregister_region(base);
    regions_.push_back({base, size, 1});
}

void ExecutableMemoryMap::unregister_region(std::uint64_t base) {
    regions_.erase(std::remove_if(regions_.begin(), regions_.end(), [base](const ExecutableRegion& r) { return r.base == base; }), regions_.end());
}

void ExecutableMemoryMap::invalidate_write(std::uint64_t address, std::size_t size) {
    for (ExecutableRegion& region : regions_) if (region.overlaps(address, size)) ++region.generation;
}

const ExecutableRegion* ExecutableMemoryMap::find(std::uint64_t address) const {
    for (const ExecutableRegion& region : regions_) if (region.contains(address)) return &region;
    return nullptr;
}

} // namespace avxlab
