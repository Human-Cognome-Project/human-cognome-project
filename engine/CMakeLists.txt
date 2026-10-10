cmake_minimum_required(VERSION 3.20)
project(engine_workspace LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)
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

# Observation of the field model: reads downloaded host state and measures
# it. Launches nothing and writes no engine state.
add_library(field_monitor STATIC src/monitor/field_monitor.cpp)
target_include_directories(field_monitor PUBLIC src)
target_link_libraries(field_monitor PUBLIC field_core)
target_compile_options(field_monitor PRIVATE -Wall -Wextra -Wno-unused-parameter)

add_executable(engine_devices apps/engine_devices.cpp)
target_link_libraries(engine_devices PRIVATE engine_support)

add_executable(field_probe apps/field_probe.cpp)
target_link_libraries(field_probe PRIVATE field_monitor)
target_compile_options(field_probe PRIVATE -Wall -Wextra -Wno-unused-parameter)

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

add_executable(monitor_test tests/monitor_test.cpp)
target_include_directories(monitor_test PRIVATE tests)
target_link_libraries(monitor_test PRIVATE field_monitor)
add_test(NAME monitor_test COMMAND monitor_test)
set_tests_properties(monitor_test PROPERTIES
    ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 900)

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
