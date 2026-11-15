#pragma once

#include "avx_runtime/ir.hpp"
#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace avxlab {

class BlockCache {
public:
    void insert(IRBlock block, std::vector<std::uint8_t> source_bytes);
    const IRBlock* find(std::uint64_t start_address) const;
    void invalidate_range(std::uint64_t address, std::size_t size);
    void clear();
    std::size_t size() const { return blocks_.size(); }

private:
    struct Entry { IRBlock block; std::vector<std::uint8_t> source_bytes; };
    std::map<std::uint64_t, Entry> blocks_;
};

} // namespace avxlab
