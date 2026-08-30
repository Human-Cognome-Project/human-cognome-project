cmake_minimum_required(VERSION 3.16)
project(byte_floor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-O2 -Wall -Wextra)

# Standalone reference + vet harness. The resolver (bytefloor.cpp) is the kernel-level
# core; its hot passes (null histogram, UTF-8 validity scan) are the AZSL-port candidates
# when this moves to GPU compute (HCP claim 579). Gem integration later.
add_executable(test_bytefloor bytefloor.cpp test_bytefloor.cpp)
add_executable(bf bytefloor.cpp bf.cpp)
