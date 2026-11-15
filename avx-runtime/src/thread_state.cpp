#include "avx_runtime/thread_state.hpp"

namespace avxlab {

GuestThreadState& ThreadStateRegistry::get_or_create(std::uint64_t thread_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    GuestThreadState& state = states_[thread_id];
    if (!state.initialized) {
        state.gpr.fill(0);
        for (auto& register_bytes : state.ymm) register_bytes.fill(0);
        state.mxcsr = 0x1F80;
        state.xcr0 = 1;
        state.initialized = true;
    }
    return state;
}

bool ThreadStateRegistry::erase(std::uint64_t thread_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return states_.erase(thread_id) != 0;
}

bool ThreadStateRegistry::contains(std::uint64_t thread_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return states_.find(thread_id) != states_.end();
}

std::size_t ThreadStateRegistry::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return states_.size();
}

std::vector<std::uint64_t> ThreadStateRegistry::thread_ids() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::uint64_t> ids;
    ids.reserve(states_.size());
    for (const auto& entry : states_) ids.push_back(entry.first);
    return ids;
}

} // namespace avxlab
