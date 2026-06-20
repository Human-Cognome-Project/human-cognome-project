cmake_minimum_required(VERSION 3.16)
project(hcp_forces CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Standalone reference + test for the STANDARD Newtonian force-generator library
# (Section 1 of docs/04-engine/newtonian-force-library.md). ForceKernel.h is the
# portable, header-only CPU reference/oracle the eventual AZSL force pass is
# validated against. No external libs needed (no lmdb/libpq). See README.md.
enable_testing()

# ---- CPU reference tests (always built; no GPU/O3DE/external deps needed) ----
add_executable(test_forces test_forces.cpp)
target_compile_options(test_forces PRIVATE -O2 -Wall -Wextra)
add_test(NAME test_forces COMMAND test_forces)
