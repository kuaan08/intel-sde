#include "avx_runtime/block_cache.hpp"

#include <utility>

namespace avxlab {

void BlockCache::insert(IRBlock block, std::vector<std::uint8_t> source_bytes) {
    blocks_[block.start_address] = Entry{std::move(block), std::move(source_bytes)};
}

const IRBlock* BlockCache::find(std::uint64_t start_address) const {
    const auto it = blocks_.find(start_address);
    return it == blocks_.end() ? nullptr : &it->second.block;
}

void BlockCache::invalidate_range(std::uint64_t address, std::size_t size) {
    const std::uint64_t end = address + static_cast<std::uint64_t>(size);
    for (auto it = blocks_.begin(); it != blocks_.end();) {
        const std::uint64_t block_start = it->second.block.start_address;
        const std::uint64_t block_end = block_start + it->second.source_bytes.size();
        if (block_start < end && address < block_end) it = blocks_.erase(it);
        else ++it;
    }
}

void BlockCache::clear() { blocks_.clear(); }

} // namespace avxlab
