#pragma once

#include "avx_runtime/ir.hpp"
#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <vector>

namespace avxlab {

struct BlockCacheKey {
    std::uint64_t module_id = 0;
    std::uint64_t guest_address = 0;
    std::uint64_t source_hash = 0;
    std::uint64_t feature_profile = 0;
    std::uint64_t page_generation = 0;
    bool operator<(const BlockCacheKey& other) const;
};

class BlockCache {
public:
    void insert(IRBlock block, std::vector<std::uint8_t> source_bytes);
    void insert(IRBlock block, std::vector<std::uint8_t> source_bytes,
                std::uint64_t module_id, std::uint64_t feature_profile,
                std::uint64_t page_generation);
    const IRBlock* find(std::uint64_t start_address) const;
    const IRBlock* find(const BlockCacheKey& key) const;
    void invalidate_range(std::uint64_t address, std::size_t size);
    void clear();
    std::size_t size() const;

private:
    struct Entry { IRBlock block; std::vector<std::uint8_t> source_bytes; BlockCacheKey key; };
    mutable std::mutex mutex_;
    std::map<BlockCacheKey, Entry> blocks_;
};

} // namespace avxlab
