cmake_minimum_required(VERSION 3.16)
project(SDE_AVX_Lab_AVX1_SSE_POC LANGUAGES CXX)

add_executable(avx1_sse_poc src/avx1_sse_poc.cpp)
target_compile_features(avx1_sse_poc PRIVATE cxx_std_17)

if(MSVC)
  target_compile_options(avx1_sse_poc PRIVATE /W4 /fp:precise)
else()
  target_compile_options(avx1_sse_poc PRIVATE
    -Wall -Wextra -Wpedantic
    -march=x86-64 -msse4.2 -mno-avx -mno-avx2 -mno-fma -mno-avx512f
    -fno-tree-vectorize -fno-lto)
endif()
