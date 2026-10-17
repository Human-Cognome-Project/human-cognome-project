cmake_minimum_required(VERSION 3.20)
project(engine_workspace LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)
include(src/assembly/membership_fields.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FindTaichiEngine)

# Engine plumbing only: instance lifetime, kernel compile/launch, and bulk
# host transfer. Nothing here decides what is computed.
add_library(engine_support STATIC
    src/engine/runtime.cpp
    src/engine/transfer.cpp
    src/engine/device.cpp)
target_include_directories(engine_support PUBLIC src)
target_link_libraries(engine_support PUBLIC engine::taichi)
# Engine headers are noisy under -Wall; keep this workspace's sources strict.
target_compile_options(engine_support PRIVATE -Wall -Wextra -Wno-unused-parameter)

# The field model: one law, the tick that runs it, and the state it runs on.
# A separate target, linking the engine rather than living inside it.
add_library(field_core STATIC src/field/field.cpp)
target_include_directories(field_core PUBLIC src)
target_link_libraries(field_core PUBLIC engine_support)
target_compile_options(field_core PRIVATE -Wall -Wextra -Wno-unused-parameter)

# Fixed-study host preparation/control, separate from the field kernels.
add_library(study_assembly STATIC src/assembly/study.cpp)
target_include_directories(study_assembly PUBLIC src)
target_link_libraries(study_assembly PUBLIC field_core)
target_compile_options(study_assembly PRIVATE -Wall -Wextra -Wno-unused-parameter)

add_library(active_workbench STATIC src/assembly/active_workbench.cpp)
target_link_libraries(active_workbench PUBLIC membership_fields study_assembly)
target_compile_options(active_workbench PRIVATE -Wall -Wextra -Wno-unused-parameter)

add_executable(active_workbench_test tests/active_workbench_test.cpp)
target_link_libraries(active_workbench_test PRIVATE active_workbench)
add_test(NAME active_workbench_test COMMAND active_workbench_test)
set_tests_properties(active_workbench_test PROPERTIES
    ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 180)

add_executable(active_transfer_experiment tests/active_transfer_experiment.cpp)
target_link_libraries(active_transfer_experiment PRIVATE active_workbench)
add_test(NAME active_transfer_experiment COMMAND active_transfer_experiment)
set_tests_properties(active_transfer_experiment PROPERTIES
    ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 240)

option(HCP_WITH_WARM_INTEGRATION "Build PostgreSQL warm-to-active integration client" OFF)
if(HCP_WITH_WARM_INTEGRATION)
  include(../kernels/database/warm/toolbox.cmake)
  add_executable(warm_active_integration tests/warm_active_integration.cpp)
  target_link_libraries(warm_active_integration PRIVATE active_workbench warm_toolbox)
endif()

add_executable(engine_devices apps/engine_devices.cpp)
target_link_libraries(engine_devices PRIVATE engine_support)

enable_testing()
add_executable(engine_smoke_test tests/engine_smoke_test.cpp)
target_include_directories(engine_smoke_test PRIVATE tests)
target_link_libraries(engine_smoke_test PRIVATE engine_support)
add_test(NAME engine_smoke_test COMMAND engine_smoke_test)
set_tests_properties(engine_smoke_test PROPERTIES
    ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 900)

add_executable(field_test tests/field_test.cpp)
target_include_directories(field_test PRIVATE tests)
target_link_libraries(field_test PRIVATE field_core)
add_test(NAME field_test COMMAND field_test)
set_tests_properties(field_test PROPERTIES
    ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 900)

foreach(suite preparation_test assembly_test)
  add_executable(${suite} tests/${suite}.cpp)
  target_include_directories(${suite} PRIVATE tests)
  target_link_libraries(${suite} PRIVATE study_assembly)
  add_test(NAME ${suite} COMMAND ${suite})
  set_tests_properties(${suite} PROPERTIES
      ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 900)
endforeach()

add_executable(letter_projection_test tests/letter_projection_test.cpp)
target_link_libraries(letter_projection_test PRIVATE letter_projection)
add_test(NAME letter_projection_test COMMAND letter_projection_test)

add_executable(letter_projection_native_test tests/letter_projection_native_test.cpp)
target_include_directories(letter_projection_native_test PRIVATE tests)
target_link_libraries(letter_projection_native_test PRIVATE letter_projection study_assembly)
add_test(NAME letter_projection_native_test COMMAND letter_projection_native_test)
set_tests_properties(letter_projection_native_test PROPERTIES
    ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 60)

add_executable(membership_fields_test tests/membership_fields_test.cpp)
target_link_libraries(membership_fields_test PRIVATE membership_fields)
add_test(NAME membership_fields_test COMMAND membership_fields_test)

add_executable(membership_fields_native_test tests/membership_fields_native_test.cpp)
target_link_libraries(membership_fields_native_test PRIVATE membership_fields study_assembly)
add_test(NAME membership_fields_native_test COMMAND membership_fields_native_test)
set_tests_properties(membership_fields_native_test PROPERTIES
    ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 120)

# Proves the >2^31 flattened-index cap is lifted: a 2-axis dense ndarray
# with N chosen so a single cell's flattened offset clears 2^31, written and
# read back through exactly one cell (no bulk iteration).
add_executable(index_cap_test tests/index_cap_test.cpp)
target_include_directories(index_cap_test PRIVATE tests)
target_link_libraries(index_cap_test PRIVATE engine_support)
add_test(NAME index_cap_test COMMAND index_cap_test)
set_tests_properties(index_cap_test PROPERTIES
    ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 900)

# Every run needs TI_LIB_DIR pointing at the engine's LLVM runtime bitcode.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/scripts/engine-env.sh.in"
               "${CMAKE_CURRENT_BINARY_DIR}/engine-env.sh" @ONLY)
