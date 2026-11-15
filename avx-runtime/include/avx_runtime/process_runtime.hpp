#pragma once

#include "avx_runtime/executable_memory.hpp"
#include "avx_runtime/feature_policy.hpp"
#include "avx_runtime/thread_state.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace avxlab {

struct UnsupportedInstructionReport {
    std::uint64_t thread_id = 0;
    std::uint64_t instruction_pointer = 0;
    std::vector<std::uint8_t> bytes;
    std::string reason;
    std::string module;
};

class ProcessRuntimeTracker {
public:
    // On Windows, refresh() enumerates committed executable VirtualQuery regions.
    // On non-Windows it is a deterministic no-op used by cross-platform tests.
    std::size_t refresh();
    void register_executable_region(std::uint64_t base, std::size_t size);
    void unregister_executable_region(std::uint64_t base);
    void notify_code_write(std::uint64_t address, std::size_t size);
    std::uint64_t generation_at(std::uint64_t address) const;
    const ExecutableMemoryMap& executable_memory() const { return executable_memory_; }

    ThreadStateRegistry& threads() { return threads_; }
    const ThreadStateRegistry& threads() const { return threads_; }

    void add_diagnostic(UnsupportedInstructionReport report);
    const std::vector<UnsupportedInstructionReport>& diagnostics() const { return diagnostics_; }
    void clear_diagnostics() { diagnostics_.clear(); }

private:
    ExecutableMemoryMap executable_memory_;
    ThreadStateRegistry threads_;
    std::vector<UnsupportedInstructionReport> diagnostics_;
};

bool guest_feature_enabled(const Coverage& coverage, const std::string& feature);

} // namespace avxlab
