# Locates the modified Taichi engine and exposes it as engine::taichi.
#
# The HCP wrapper consumes a Taichi build tree rather than an install because
# the headers, component archives and generated LLVM runtime bitcode must be one
# matched set.
#
# Inputs (all cache variables, overridable on the command line):
#   ENGINE_TAICHI_ROOT   Taichi source checkout
#                        (default: ${engine workspace}/taichi)
#   ENGINE_TAICHI_BUILD  matching Taichi build tree
#                        (default: ${ROOT}/build-review)
#   ENGINE_TAICHI_RUNTIME_SOURCE
#                        generated LLVM runtime bitcode directory
#   ENGINE_TAICHI_CUDA   AUTO, ON or OFF. Must agree with the matched Taichi
#                        build; AUTO reads that build's CUDA configuration.
#   ENGINE_LLVM_CONFIG   llvm-config used by the Taichi build
#
# Outputs:
#   engine::taichi
#   ENGINE_TAICHI_RUNTIME_DIR   value for TI_LIB_DIR
#   ENGINE_TAICHI_REVISION      source identity (subtree SHA in the monorepo)
#   ENGINE_TAICHI_WITH_CUDA     TRUE iff the matched build includes CUDA
#
# Backend policy belongs to the Taichi build. The HCP model does not change
# between CPU and CUDA; this wrapper only mirrors the capabilities actually
# present in the matched Taichi build.

set(ENGINE_TAICHI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/taichi" CACHE PATH
    "Taichi engine checkout")
set(ENGINE_TAICHI_BUILD "${ENGINE_TAICHI_ROOT}/build-review" CACHE PATH
    "Taichi engine build tree")
set(ENGINE_TAICHI_RUNTIME_SOURCE
    "${ENGINE_TAICHI_ROOT}/taichi/runtime/llvm/runtime_module" CACHE PATH
    "Generated LLVM runtime bitcode directory")
set(ENGINE_TAICHI_CUDA "AUTO" CACHE STRING
    "Use CUDA components from the matched Taichi build: AUTO, ON or OFF")
set_property(CACHE ENGINE_TAICHI_CUDA PROPERTY STRINGS AUTO ON OFF)
set(ENGINE_LLVM_CONFIG "/usr/lib/llvm-15/bin/llvm-config" CACHE FILEPATH
    "llvm-config used to build the engine")

foreach(probe
    "${ENGINE_TAICHI_ROOT}/taichi/program/program.h"
    "${ENGINE_TAICHI_ROOT}/taichi/ir/ir_builder.h"
    "${ENGINE_TAICHI_BUILD}/libtaichi_core_static.a"
    "${ENGINE_TAICHI_RUNTIME_SOURCE}/runtime_x64.bc")
  if(NOT EXISTS "${probe}")
    message(FATAL_ERROR
      "Engine incomplete: ${probe} is missing. Build the Taichi fork first "
      "or set ENGINE_TAICHI_ROOT/ENGINE_TAICHI_BUILD.")
  endif()
endforeach()

# The engine splits its object code across per-component archives; the merged
# taichi_core archive does not contain them.
set(_engine_components
    taichi/common/libtaichi_common.a
    taichi/util/libtaichi_util.a
    taichi/compilation_manager/libcompilation_manager.a
    taichi/codegen/cpu/libcpu_codegen.a
    taichi/codegen/llvm/libllvm_codegen.a
    taichi/runtime/cpu/libcpu_runtime.a
    taichi/runtime/llvm/libllvm_runtime.a
    taichi/runtime/program_impls/llvm/libllvm_program_impl.a
    taichi/rhi/libti_device_api.a
    taichi/rhi/common/libcommon_rhi.a
    taichi/rhi/cpu/libcpu_rhi.a
    taichi/rhi/llvm/libllvm_rhi.a
    taichi/rhi/interop/libinterop_rhi.a)

set(_engine_cuda_components
    taichi/codegen/cuda/libcuda_codegen.a
    taichi/runtime/cuda/libcuda_runtime.a
    taichi/rhi/cuda/libcuda_rhi.a)

# Archives from an earlier configuration can remain in a reused build tree.
# The core archive's compile definitions, rather than those stale files,
# decide which backend dependencies must be linked.
set(_engine_taichi_cache "${ENGINE_TAICHI_BUILD}/CMakeCache.txt")
if(NOT EXISTS "${_engine_taichi_cache}")
  message(FATAL_ERROR "Matched Taichi build has no CMakeCache.txt at ${_engine_taichi_cache}")
endif()
file(STRINGS "${_engine_taichi_cache}" _engine_cuda_cache
     REGEX "^TI_WITH_CUDA:BOOL=(ON|OFF)$")
if(NOT _engine_cuda_cache)
  message(FATAL_ERROR "Matched Taichi build has no TI_WITH_CUDA:BOOL setting")
endif()
set(_engine_taichi_cuda FALSE)
if(_engine_cuda_cache STREQUAL "TI_WITH_CUDA:BOOL=ON")
  set(_engine_taichi_cuda TRUE)
endif()

set(_engine_cuda_complete TRUE)
foreach(component IN LISTS _engine_cuda_components)
  if(NOT EXISTS "${ENGINE_TAICHI_BUILD}/${component}")
    set(_engine_cuda_complete FALSE)
  endif()
endforeach()
if(NOT EXISTS "${ENGINE_TAICHI_RUNTIME_SOURCE}/runtime_cuda.bc")
  set(_engine_cuda_complete FALSE)
endif()
if(NOT EXISTS "${ENGINE_TAICHI_ROOT}/external/cuda_libdevice/slim_libdevice.10.bc")
  set(_engine_cuda_complete FALSE)
endif()

string(TOUPPER "${ENGINE_TAICHI_CUDA}" _engine_cuda_mode)
if(_engine_cuda_mode STREQUAL "AUTO")
  set(ENGINE_TAICHI_WITH_CUDA ${_engine_taichi_cuda})
elseif(_engine_cuda_mode STREQUAL "ON")
  if(NOT _engine_taichi_cuda)
    message(FATAL_ERROR "ENGINE_TAICHI_CUDA=ON requires a CUDA-enabled Taichi build")
  endif()
  set(ENGINE_TAICHI_WITH_CUDA TRUE)
elseif(_engine_cuda_mode STREQUAL "OFF")
  if(_engine_taichi_cuda)
    message(FATAL_ERROR
      "ENGINE_TAICHI_CUDA=OFF requires Taichi built with TI_WITH_CUDA=OFF; "
      "use a separate CPU Taichi build tree instead")
  endif()
  set(ENGINE_TAICHI_WITH_CUDA FALSE)
else()
  message(FATAL_ERROR "ENGINE_TAICHI_CUDA must be AUTO, ON or OFF")
endif()

if(ENGINE_TAICHI_WITH_CUDA)
  if(NOT _engine_cuda_complete)
    message(FATAL_ERROR
      "CUDA-enabled Taichi build is missing CUDA component archives/runtime "
      "artifacts; rebuild the matched Taichi tree")
  endif()
  list(APPEND _engine_components ${_engine_cuda_components})
endif()

set(_engine_libs "${ENGINE_TAICHI_BUILD}/libtaichi_core_static.a")
foreach(component IN LISTS _engine_components)
  if(NOT EXISTS "${ENGINE_TAICHI_BUILD}/${component}")
    message(FATAL_ERROR
      "Engine build tree is missing ${component}; rebuild the engine.")
  endif()
  list(APPEND _engine_libs "${ENGINE_TAICHI_BUILD}/${component}")
endforeach()

if(NOT EXISTS "${ENGINE_LLVM_CONFIG}")
  message(FATAL_ERROR "llvm-config not found at ${ENGINE_LLVM_CONFIG}")
endif()
execute_process(COMMAND "${ENGINE_LLVM_CONFIG}" --includedir
                OUTPUT_VARIABLE ENGINE_LLVM_INCLUDE_DIR
                OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "${ENGINE_LLVM_CONFIG}" --libdir
                OUTPUT_VARIABLE ENGINE_LLVM_LIB_DIR
                OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "${ENGINE_LLVM_CONFIG}" --version
                OUTPUT_VARIABLE ENGINE_LLVM_VERSION
                OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "${ENGINE_LLVM_CONFIG}" --libs
                OUTPUT_VARIABLE _engine_llvm_libs
                OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "${ENGINE_LLVM_CONFIG}" --system-libs
                OUTPUT_VARIABLE _engine_llvm_system_libs
                OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(_engine_llvm_libs UNIX_COMMAND "${_engine_llvm_libs}")
separate_arguments(_engine_llvm_system_libs UNIX_COMMAND
                   "${_engine_llvm_system_libs}")

if(NOT ENGINE_LLVM_VERSION VERSION_GREATER_EQUAL 15)
  message(FATAL_ERROR
    "The recovered engine was built against LLVM 15; "
    "${ENGINE_LLVM_CONFIG} reports ${ENGINE_LLVM_VERSION}.")
endif()

# In the HCP monorepo, git -C engine/taichi rev-parse HEAD would report the
# enclosing HCP commit. Prefer the actual Taichi subtree object. If callers
# point ENGINE_TAICHI_ROOT at a standalone checkout, fall back to its HEAD.
execute_process(
  COMMAND git -C "${ENGINE_TAICHI_ROOT}" rev-parse HEAD:engine/taichi
  RESULT_VARIABLE _engine_subtree_result
  OUTPUT_VARIABLE ENGINE_TAICHI_REVISION
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET)
if(NOT _engine_subtree_result EQUAL 0 OR NOT ENGINE_TAICHI_REVISION)
  execute_process(
    COMMAND git -C "${ENGINE_TAICHI_ROOT}" rev-parse HEAD
    OUTPUT_VARIABLE ENGINE_TAICHI_REVISION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
endif()
if(NOT ENGINE_TAICHI_REVISION)
  set(ENGINE_TAICHI_REVISION "unknown")
endif()

add_library(engine_taichi INTERFACE)
add_library(engine::taichi ALIAS engine_taichi)

target_include_directories(engine_taichi SYSTEM INTERFACE
    "${ENGINE_TAICHI_ROOT}"
    "${ENGINE_TAICHI_ROOT}/external/include"
    "${ENGINE_TAICHI_ROOT}/external/spdlog/include"
    "${ENGINE_TAICHI_ROOT}/external/eigen"
    "${ENGINE_TAICHI_ROOT}/external/FP16/include"
    "${ENGINE_TAICHI_ROOT}/external/PicoSHA2"
    "${ENGINE_TAICHI_ROOT}/external/SPIRV-Tools/include"
    "${ENGINE_LLVM_INCLUDE_DIR}")

target_compile_definitions(engine_taichi INTERFACE
    TI_INCLUDED TI_WITH_LLVM TI_ARCH_x64 TI_ISE_NONE
    _GNU_SOURCE __STDC_CONSTANT_MACROS __STDC_FORMAT_MACROS __STDC_LIMIT_MACROS)
if(ENGINE_TAICHI_WITH_CUDA)
  target_compile_definitions(engine_taichi INTERFACE TI_WITH_CUDA)
endif()

target_link_libraries(engine_taichi INTERFACE
    -Wl,--start-group ${_engine_libs} -Wl,--end-group
    "-L${ENGINE_LLVM_LIB_DIR}" ${_engine_llvm_libs}
    ${_engine_llvm_system_libs}
    Threads::Threads ${CMAKE_DL_LIBS})

# TI_LIB_DIR is simply a native runtime-artifact lookup. Stage only the
# artifacts for backends present in this matched build; no Python package
# directory is required.
set(ENGINE_TAICHI_RUNTIME_DIR "${CMAKE_CURRENT_BINARY_DIR}/taichi-runtime")
file(MAKE_DIRECTORY "${ENGINE_TAICHI_RUNTIME_DIR}")
configure_file("${ENGINE_TAICHI_RUNTIME_SOURCE}/runtime_x64.bc"
               "${ENGINE_TAICHI_RUNTIME_DIR}/runtime_x64.bc" COPYONLY)
if(ENGINE_TAICHI_WITH_CUDA)
  configure_file("${ENGINE_TAICHI_RUNTIME_SOURCE}/runtime_cuda.bc"
                 "${ENGINE_TAICHI_RUNTIME_DIR}/runtime_cuda.bc" COPYONLY)
  configure_file(
    "${ENGINE_TAICHI_ROOT}/external/cuda_libdevice/slim_libdevice.10.bc"
    "${ENGINE_TAICHI_RUNTIME_DIR}/slim_libdevice.10.bc" COPYONLY)
endif()

message(STATUS "Engine checkout : ${ENGINE_TAICHI_ROOT} (${ENGINE_TAICHI_REVISION})")
message(STATUS "Engine build    : ${ENGINE_TAICHI_BUILD}")
message(STATUS "Engine LLVM     : ${ENGINE_LLVM_VERSION} (${ENGINE_LLVM_LIB_DIR})")
message(STATUS "Engine CUDA     : ${ENGINE_TAICHI_WITH_CUDA}")
message(STATUS "Engine runtime source : ${ENGINE_TAICHI_RUNTIME_SOURCE}")
message(STATUS "Engine runtime stage  : ${ENGINE_TAICHI_RUNTIME_DIR}")
