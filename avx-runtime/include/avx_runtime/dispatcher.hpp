#pragma once

#include "avx_runtime/block_cache.hpp"
#include "avx_runtime/process_runtime.hpp"
#include "avx_runtime/translator.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace avxlab {

enum class DispatchStatus { Translated, CacheHit, NoCode, Unsupported };

struct DispatchRequest {
    std::uint64_t module_id = 0;
    std::uint64_t thread_id = 0;
    std::uint64_t guest_address = 0;
    std::uint64_t feature_profile = 0;
    std::size_t max_instructions = 64;
    std::size_t max_bytes = 4096;
    std::string module;
};

struct DispatchResult {
    DispatchStatus status = DispatchStatus::NoCode;
    IRBlock block;
    std::size_t source_length = 0;
    std::string diagnostic;
};

using GuestCodeReader = std::function<bool(std::uint64_t address, std::size_t size,
                                            std::vector<std::uint8_t>* bytes)>;

class TranslationDispatcher {
public:
    explicit TranslationDispatcher(ProcessRuntimeTracker* runtime = nullptr);

    DispatchResult dispatch(const DispatchRequest& request, const GuestCodeReader& reader);
    void invalidate_code(std::uint64_t address, std::size_t size);
    void clear();
    std::size_t cache_size() const { return cache_.size(); }

private:
    ProcessRuntimeTracker owned_runtime_;
    ProcessRuntimeTracker* runtime_ = nullptr;
    BlockCache cache_;
};

} // namespace avxlab
