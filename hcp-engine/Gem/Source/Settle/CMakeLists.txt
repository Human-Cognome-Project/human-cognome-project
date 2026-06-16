cmake_minimum_required(VERSION 3.16)
project(hcp_settle CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-O2 -Wall -Wextra)

# Standalone reference + test harness for the AZSL settle port (slice 1).
# SettleKernel.h is the portable CPU reference and the deterministic ORACLE the
# AZSL compute shader (HCPSettleCompute.azsl) is validated against. Gem/O3DE
# ComputePass integration comes in a later slice (corpus 02). See README.md.
add_executable(test_settle test_settle.cpp)

enable_testing()
add_test(NAME test_settle COMMAND test_settle)
