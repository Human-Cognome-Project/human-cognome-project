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
    src/engine/precision.cpp
    src/engine/record_map.cpp
    src/engine/transfer.cpp
    src/engine/device.cpp)
target_include_directories(engine_support PUBLIC src)
target_link_libraries(engine_support PUBLIC engine::taichi)
# Engine headers are noisy under -Wall; keep this workspace's sources strict.
target_compile_options(engine_support PRIVATE -Wall -Wextra -Wno-unused-parameter)

# The field model: one law, the tick that runs it, and the state it runs on.
# A separate target, linking the engine rather than living inside it.
add_library(field_core STATIC src/field/field.cpp src/field/activity.cpp
    src/field/prepared_capacity.cpp src/field/prepared_storage.cpp
    src/field/prepared_update.cpp src/field/prepared_geometry.cpp
    src/field/centroid_cascade.cpp)
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
add_executable(precision_test tests/precision_test.cpp)
target_link_libraries(precision_test PRIVATE engine_support)
foreach(mode bulk direct)
  if(mode STREQUAL "direct")
    set(precision_args --direct)
  else()
    set(precision_args)
  endif()
  add_test(NAME precision_${mode} COMMAND precision_test ${precision_args})
  set_tests_properties(precision_${mode} PROPERTIES
      ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 300)
endforeach()
add_executable(prepared_launch_test tests/prepared_launch_test.cpp)
target_link_libraries(prepared_launch_test PRIVATE engine_support)
add_test(NAME prepared_launch_test COMMAND prepared_launch_test)
set_tests_properties(prepared_launch_test PROPERTIES
    ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 180)
add_executable(prepared_execution_test tests/prepared_execution_test.cpp)
target_include_directories(prepared_execution_test PRIVATE tests)
target_link_libraries(prepared_execution_test PRIVATE study_assembly membership_fields)
foreach(mode padded direct large large-direct letters letters-direct)
  set(mode_args)
  if(mode MATCHES "large")
    list(APPEND mode_args --large)
  endif()
  if(mode MATCHES "letters")
    list(APPEND mode_args --letters)
  endif()
  if(mode MATCHES "direct")
    list(APPEND mode_args --direct)
  endif()
  add_test(NAME prepared_execution_${mode} COMMAND prepared_execution_test ${mode_args})
  set_tests_properties(prepared_execution_${mode} PROPERTIES
      ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 300)
endforeach()

add_executable(prepared_update_test tests/prepared_update_test.cpp)
target_include_directories(prepared_update_test PRIVATE tests)
target_link_libraries(prepared_update_test PRIVATE study_assembly membership_fields)
foreach(mode padded direct large large-direct letters letters-direct)
  set(mode_args)
  if(mode MATCHES "large")
    list(APPEND mode_args --large)
  endif()
  if(mode MATCHES "letters")
    list(APPEND mode_args --letters)
  endif()
  if(mode MATCHES "direct")
    list(APPEND mode_args --direct)
  endif()
  add_test(NAME prepared_update_${mode} COMMAND prepared_update_test ${mode_args})
  set_tests_properties(prepared_update_${mode} PROPERTIES
      ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 300)
endforeach()

add_executable(prepared_geometry_test tests/prepared_geometry_test.cpp)
target_link_libraries(prepared_geometry_test PRIVATE field_core)
foreach(mode padded direct large large-direct maximum maximum-direct large-maximum large-maximum-direct)
  set(mode_args)
  if(mode MATCHES "large")
    list(APPEND mode_args --large)
  endif()
  if(mode MATCHES "maximum")
    list(APPEND mode_args --maximum)
  endif()
  if(mode MATCHES "direct")
    list(APPEND mode_args --direct)
  endif()
  add_test(NAME prepared_geometry_${mode} COMMAND prepared_geometry_test ${mode_args})
  set_tests_properties(prepared_geometry_${mode} PROPERTIES
      ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 300)
endforeach()

add_executable(cascade_test tests/cascade_test.cpp)
target_link_libraries(cascade_test PRIVATE study_assembly)
foreach(mode padded direct large large-direct maximum maximum-direct large-maximum large-maximum-direct)
  set(cascade_args)
  if(mode MATCHES "direct")
    list(APPEND cascade_args --direct)
  endif()
  if(mode MATCHES "large")
    list(APPEND cascade_args --large)
  endif()
  if(mode MATCHES "maximum")
    list(APPEND cascade_args --maximum)
  endif()
  add_test(NAME cascade_${mode} COMMAND cascade_test ${cascade_args})
  set_tests_properties(cascade_${mode} PROPERTIES
      ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 300)
endforeach()
add_executable(prepared_capacity_test tests/prepared_capacity_test.cpp)
target_link_libraries(prepared_capacity_test PRIVATE field_core)
add_test(NAME prepared_capacity_test COMMAND prepared_capacity_test)

add_executable(record_map_test tests/record_map_test.cpp)
target_link_libraries(record_map_test PRIVATE engine_support)
add_test(NAME record_map_padded COMMAND record_map_test --records 70 --span 1048576 --cycles 32)
add_test(NAME record_map_direct COMMAND record_map_test --records 70 --span 1048576 --cycles 32 --direct)
add_test(NAME record_map_working_set COMMAND record_map_test --records 4096 --span 16777216 --cycles 32)
set_tests_properties(record_map_padded record_map_direct record_map_working_set PROPERTIES
    ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 300)

add_executable(native_pool_test tests/native_pool_test.cpp)
target_link_libraries(native_pool_test PRIVATE engine_support)
add_test(NAME native_pool_test COMMAND native_pool_test)
set_tests_properties(native_pool_test PROPERTIES TIMEOUT 30)

add_executable(prepared_space_probe tests/prepared_space_probe.cpp)
target_link_libraries(prepared_space_probe PRIVATE engine_support)
foreach(layout dense-budgeted flat-bitmasked pointer-cold pointer-retained pointer-recycled)
  add_test(NAME prepared_space_${layout}
      COMMAND prepared_space_probe --layout ${layout} --span 70 --repeats 3)
  set_tests_properties(prepared_space_${layout} PROPERTIES
      ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 180)
endforeach()

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

add_executable(activity_test tests/activity_test.cpp)
add_executable(native_activity_test tests/native_activity_test.cpp)
target_include_directories(native_activity_test PRIVATE tests)
target_link_libraries(native_activity_test PRIVATE field_core)
add_test(NAME native_activity_test COMMAND native_activity_test)
set_tests_properties(native_activity_test PROPERTIES
    ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 180)
target_include_directories(activity_test PRIVATE tests)
target_link_libraries(activity_test PRIVATE field_core)
add_test(NAME activity_test COMMAND activity_test)
add_test(NAME activity_parallel_test COMMAND activity_test --parallel)
add_test(NAME activity_direct_test COMMAND activity_test --direct --parallel)
set_tests_properties(activity_test activity_parallel_test activity_direct_test PROPERTIES
    ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 180)

add_executable(activity_assembly_test tests/activity_assembly_test.cpp)
target_include_directories(activity_assembly_test PRIVATE tests)
target_link_libraries(activity_assembly_test PRIVATE membership_fields study_assembly)
add_test(NAME activity_assembly_test COMMAND activity_assembly_test)
add_test(NAME activity_assembly_direct_test COMMAND activity_assembly_test --direct)
set_tests_properties(activity_assembly_test activity_assembly_direct_test PROPERTIES
    ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 180)

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
add_executable(structural_recipe_test tests/structural_recipe_test.cpp)
target_link_libraries(structural_recipe_test PRIVATE membership_fields)
add_test(NAME structural_recipe_test COMMAND structural_recipe_test)

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

# Only for a CUDA-enabled Taichi build (CI's CPU-only build does not have it):
# a requested CUDA instance must report the backend it actually runs on.
if(ENGINE_TAICHI_WITH_CUDA)
  add_executable(runtime_backend_test tests/runtime_backend_test.cpp)
  target_include_directories(runtime_backend_test PRIVATE tests)
  target_link_libraries(runtime_backend_test PRIVATE engine_support)
  foreach(mode no-driver no-device device)
    add_test(NAME runtime_backend_${mode} COMMAND runtime_backend_test --${mode})
    set_tests_properties(runtime_backend_${mode} PROPERTIES
        ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 120)
  endforeach()
endif()

option(HCP_WITH_VIEWPORT "Build the optional offline study snapshot exporter" OFF)
if(HCP_WITH_VIEWPORT)
  add_subdirectory(../tools/viewport/native viewport)
endif()

# Every run needs TI_LIB_DIR pointing at the engine's LLVM runtime bitcode.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/scripts/engine-env.sh.in"
               "${CMAKE_CURRENT_BINARY_DIR}/engine-env.sh" @ONLY)
