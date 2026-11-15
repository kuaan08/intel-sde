#pragma once

#include <array>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace avxlab {

struct GuestThreadState {
    std::array<std::array<std::uint8_t, 32>, 16> ymm{};
    std::array<std::uint64_t, 16> gpr{};
    std::uint32_t mxcsr = 0x1F80;
    std::uint64_t xcr0 = 1;
    bool initialized = false;
};

class ThreadStateRegistry {
public:
    GuestThreadState& get_or_create(std::uint64_t thread_id);
    bool erase(std::uint64_t thread_id);
    bool contains(std::uint64_t thread_id) const;
    std::size_t size() const;
    std::vector<std::uint64_t> thread_ids() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::uint64_t, GuestThreadState> states_;
};

} // namespace avxlab
