#include "avx_runtime/dispatcher.hpp"

#include <algorithm>
#include <utility>

namespace avxlab {

TranslationDispatcher::TranslationDispatcher(ProcessRuntimeTracker* runtime)
    : runtime_(runtime ? runtime : &owned_runtime_) {}

DispatchResult TranslationDispatcher::dispatch(const DispatchRequest& request,
                                               const GuestCodeReader& reader) {
    DispatchResult result;
    if (!reader || request.guest_address == 0) {
        result.status = DispatchStatus::NoCode;
        result.diagnostic = "dispatcher requires a guest address and code reader";
        return result;
    }

    GuestThreadState& thread = runtime_->threads().get_or_create(request.thread_id);
    thread.initialized = true;

    std::vector<std::uint8_t> bytes;
    if (!reader(request.guest_address, request.max_bytes, &bytes) || bytes.empty()) {
        result.status = DispatchStatus::NoCode;
        result.diagnostic = "guest code reader returned no executable bytes";
        return result;
    }

    const BlockTranslationResult translated = translate_basic_block(
        bytes, request.guest_address, request.max_instructions, request.max_bytes);
    result.block = translated.block;
    result.source_length = translated.source_length;
    if (translated.block.instructions.empty() || translated.stopped_on_unsupported) {
        result.status = DispatchStatus::Unsupported;
        result.diagnostic = "guest block contains an unsupported or undecodable instruction";
        UnsupportedInstructionReport report;
        report.thread_id = request.thread_id;
        report.instruction_pointer = request.guest_address + translated.source_length;
        report.module = request.module;
        report.reason = result.diagnostic;
        const std::size_t offset = std::min(translated.source_length, bytes.size());
        report.bytes.assign(bytes.begin() + static_cast<std::ptrdiff_t>(offset), bytes.end());
        runtime_->add_diagnostic(std::move(report));
        return result;
    }

    std::vector<std::uint8_t> source(bytes.begin(),
                                     bytes.begin() + static_cast<std::ptrdiff_t>(
                                         std::min(translated.source_length, bytes.size())));
    const std::uint64_t generation = runtime_->generation_at(request.guest_address);
    const BlockCacheKey key{request.module_id, request.guest_address,
                            hash_block_bytes(source), request.feature_profile, generation};
    if (const std::optional<IRBlock> cached = cache_.find_copy(key)) {
        result.status = DispatchStatus::CacheHit;
        result.block = *cached;
        result.source_length = source.size();
        return result;
    }
    result.block.generation = generation;
    cache_.insert(result.block, std::move(source), request.module_id,
                  request.feature_profile, generation);
    result.status = DispatchStatus::Translated;
    return result;
}

void TranslationDispatcher::invalidate_code(std::uint64_t address, std::size_t size) {
    runtime_->notify_code_write(address, size);
    cache_.invalidate_range(address, size);
}

void TranslationDispatcher::clear() {
    cache_.clear();
    runtime_->clear_diagnostics();
}

} // namespace avxlab
