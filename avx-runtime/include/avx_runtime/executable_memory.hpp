#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace avxlab {

struct ExecutableRegion {
    std::uint64_t base = 0;
    std::size_t size = 0;
    std::uint64_t generation = 0;
    bool contains(std::uint64_t address) const { return address >= base && address - base < size; }
    bool overlaps(std::uint64_t address, std::size_t length) const;
};

class ExecutableMemoryMap {
public:
    void register_region(std::uint64_t base, std::size_t size);
    void unregister_region(std::uint64_t base);
    void invalidate_write(std::uint64_t address, std::size_t size);
    const ExecutableRegion* find(std::uint64_t address) const;
    const std::vector<ExecutableRegion>& regions() const { return regions_; }

private:
    std::vector<ExecutableRegion> regions_;
};

} // namespace avxlab
