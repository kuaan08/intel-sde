cmake_minimum_required(VERSION 3.16)
project(SDE_AVX_Lab_Runtime LANGUAGES CXX)

set(RUNTIME_CORE_SOURCES
  src/memory_address.cpp
  src/decoder.cpp
  src/dispatcher.cpp
  src/sse_lowering.cpp
  src/thread_state.cpp
  src/process_runtime.cpp
  src/block_cache.cpp
  src/block_builder.cpp
  src/executable_memory.cpp
  src/feature_policy.cpp
  src/translator.cpp
  src/windows_process_host.cpp
)

function(configure_runtime_target target)
  target_compile_features(${target} PRIVATE cxx_std_17)
  target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /O2 /arch:SSE2)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -O2 -fno-tree-vectorize -fno-lto)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
      target_compile_options(${target} PRIVATE -march=x86-64 -msse4.2 -mno-avx -mno-avx2 -mno-fma -mno-avx512f)
    endif()
  endif()
endfunction()

add_executable(avx_runtime src/avx_runtime.cpp ${RUNTIME_CORE_SOURCES})
configure_runtime_target(avx_runtime)

include(CTest)
if(BUILD_TESTING)
  set(SDE_AVX_TEST_RUNNER "" CACHE FILEPATH "Optional emulator executable, e.g. /usr/bin/qemu-x86_64")
  set(SDE_AVX_TEST_RUNNER_ARGS "" CACHE STRING "Optional space-separated arguments for the test runner")
  if(SDE_AVX_TEST_RUNNER)
    separate_arguments(SDE_AVX_TEST_RUNNER_ARGS_LIST NATIVE_COMMAND "${SDE_AVX_TEST_RUNNER_ARGS}")
    set(SDE_AVX_TEST_PREFIX ${SDE_AVX_TEST_RUNNER} ${SDE_AVX_TEST_RUNNER_ARGS_LIST})
  endif()

  add_executable(runtime_arch_tests tests/runtime_arch_tests.cpp ${RUNTIME_CORE_SOURCES})
  configure_runtime_target(runtime_arch_tests)
  add_test(NAME runtime_architecture_tests COMMAND ${SDE_AVX_TEST_PREFIX} $<TARGET_FILE:runtime_arch_tests>)
  add_test(NAME independent_avx_runtime_self_test COMMAND ${SDE_AVX_TEST_PREFIX} $<TARGET_FILE:avx_runtime> --self-test)
endif()

if(MSVC)
  # The fixture emits its small VEX.128 function into executable memory at
  # runtime, avoiding a MASM dependency while preserving exact guest bytes.
  add_executable(avx1_windows_fixture
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/windows_avx1_fixture.cpp"
  )
  target_compile_features(avx1_windows_fixture PRIVATE cxx_std_17)
  target_compile_options(avx1_windows_fixture PRIVATE /W4 /O2)

  add_executable(windows_cpuid_fixture
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/windows_cpuid_fixture.cpp"
  )
  target_compile_features(windows_cpuid_fixture PRIVATE cxx_std_17)
  target_compile_options(windows_cpuid_fixture PRIVATE /W4 /O2)

  add_library(avx_winshim SHARED src/windows_avx_shim.cpp)
  target_compile_features(avx_winshim PRIVATE cxx_std_17)
  target_compile_options(avx_winshim PRIVATE /W4 /O2 /arch:SSE2)
endif()
