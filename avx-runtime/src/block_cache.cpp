#include "avx_runtime/block_cache.hpp"

#include <limits>
#include <utility>

namespace avxlab {
namespace {
std::uint64_t hash_bytes(const std::vector<std::uint8_t>& bytes) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (const std::uint8_t byte : bytes) { hash ^= byte; hash *= 1099511628211ULL; }
    return hash;
}
}

std::uint64_t hash_block_bytes(const std::vector<std::uint8_t>& bytes) {
    return hash_bytes(bytes);
}

bool BlockCacheKey::operator<(const BlockCacheKey& other) const {
    if (module_id != other.module_id) return module_id < other.module_id;
    if (guest_address != other.guest_address) return guest_address < other.guest_address;
    if (source_hash != other.source_hash) return source_hash < other.source_hash;
    if (feature_profile != other.feature_profile) return feature_profile < other.feature_profile;
    return page_generation < other.page_generation;
}

void BlockCache::insert(IRBlock block, std::vector<std::uint8_t> source_bytes) {
    insert(std::move(block), std::move(source_bytes), 0, 0, 0);
}

void BlockCache::insert(IRBlock block, std::vector<std::uint8_t> source_bytes,
                        std::uint64_t module_id, std::uint64_t feature_profile,
                        std::uint64_t page_generation) {
    BlockCacheKey key{module_id, block.start_address, hash_block_bytes(source_bytes), feature_profile, page_generation};
    std::lock_guard<std::mutex> lock(mutex_);
    blocks_[key] = Entry{std::move(block), std::move(source_bytes), key};
}

const IRBlock* BlockCache::find(std::uint64_t start_address) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& item : blocks_) if (item.second.block.start_address == start_address) return &item.second.block;
    return nullptr;
}

const IRBlock* BlockCache::find(const BlockCacheKey& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = blocks_.find(key);
    return it == blocks_.end() ? nullptr : &it->second.block;
}

std::optional<IRBlock> BlockCache::find_copy(const BlockCacheKey& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = blocks_.find(key);
    if (it == blocks_.end()) return std::nullopt;
    return it->second.block;
}

void BlockCache::invalidate_range(std::uint64_t address, std::size_t size) {
    if (size == 0) return;
    std::lock_guard<std::mutex> lock(mutex_);
    const std::uint64_t extent = static_cast<std::uint64_t>(size);
    const std::uint64_t end = address > UINT64_MAX - extent ? UINT64_MAX : address + extent;
    for (auto it = blocks_.begin(); it != blocks_.end();) {
        const std::uint64_t block_start = it->second.block.start_address;
        const std::uint64_t length = static_cast<std::uint64_t>(it->second.source_bytes.size());
        const std::uint64_t block_end = block_start > UINT64_MAX - length ? UINT64_MAX : block_start + length;
        if (block_start < end && address < block_end) it = blocks_.erase(it);
        else ++it;
    }
}

void BlockCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    blocks_.clear();
}

std::size_t BlockCache::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return blocks_.size();
}

} // namespace avxlab
