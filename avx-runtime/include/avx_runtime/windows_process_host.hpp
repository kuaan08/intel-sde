#pragma once

#include <string>

namespace avxlab {

// Starts a Windows x64 target with the independent AVX shim injected.
// target_arguments is appended after a correctly quoted executable path.
// Returns the target exit code, or a negative value on host/injection failure.
int run_process_with_shim(const std::string& target_path, const std::string& shim_path,
                          const std::string& target_arguments = std::string());

} // namespace avxlab
