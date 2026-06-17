cmake_minimum_required(VERSION 3.16)
project(hcp_pack CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Standalone reference + test for the compact-ID packer (vocab.lmdb correction).
# PackKernel.h is the portable CPU reference/oracle; PackStore.h is the
# GPU-facing LMDB store ({compact-id -> chars} only). See README.md.
enable_testing()

# ---- CPU reference + round-trip test (always built; no GPU/O3DE needed) ----
# Needs system liblmdb (lmdb.h + liblmdb.so), same dependency the engine uses.
add_executable(test_pack test_pack.cpp)
target_compile_options(test_pack PRIVATE -O2 -Wall -Wextra)
target_link_libraries(test_pack PRIVATE lmdb)
add_test(NAME test_pack COMMAND test_pack)

# ---- Producer: build the real store from the warm set (needs libpq) ----
# Reads hcp_var.envelope_working_set (localhost:5432) and writes the corrected
# {compact-id -> chars} store. Not a ctest (needs the live warm DB); run by hand.
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBPQ libpq)
endif()
if(LIBPQ_FOUND)
    add_executable(pack_vocab pack_vocab.cpp)
    target_compile_options(pack_vocab PRIVATE -O2 -Wall -Wextra ${LIBPQ_CFLAGS})
    target_include_directories(pack_vocab PRIVATE ${LIBPQ_INCLUDE_DIRS})
    target_link_libraries(pack_vocab PRIVATE lmdb ${LIBPQ_LIBRARIES})
    message(STATUS "hcp_pack: pack_vocab producer ENABLED (libpq found)")
else()
    message(STATUS "hcp_pack: pack_vocab producer disabled (libpq not found)")
endif()
