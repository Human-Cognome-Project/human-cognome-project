# Locates the updated Taichi engine and exposes it as the target
# engine::taichi. The engine is consumed as a build tree, not as an install:
# its headers, its static libraries and its LLVM runtime bitcode are one
# matched set and must come from the same build.
#
# Inputs (cache variables, all overridable on the command line):
#   ENGINE_TAICHI_ROOT     engine source checkout        (default ${engine workspace}/taichi)
#   ENGINE_TAICHI_BUILD    engine build tree             (default ${ROOT}/build-review)
#   ENGINE_TAICHI_RUNTIME_SOURCE  generated LLVM runtime bitcode dir
#                                (default ${ROOT}/taichi/runtime/llvm/runtime_module)
#   ENGINE_LLVM_CONFIG     llvm-config of the engine's LLVM (default /usr/lib/llvm-15/bin/llvm-config)
#
# Outputs:
#   engine::taichi            imported interface target
#   ENGINE_TAICHI_RUNTIME_DIR value for the TI_LIB_DIR environment variable
#   ENGINE_TAICHI_REVISION    engine git revision, recorded for the manifest

set(ENGINE_TAICHI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/taichi" CACHE PATH "Taichi engine checkout")
set(ENGINE_TAICHI_BUILD "${ENGINE_TAICHI_ROOT}/build-review" CACHE PATH "Taichi engine build tree")
set(ENGINE_TAICHI_RUNTIME_SOURCE "${ENGINE_TAICHI_ROOT}/taichi/runtime/llvm/runtime_module" CACHE PATH "Generated LLVM runtime bitcode directory")
set(ENGINE_LLVM_CONFIG "/usr/lib/llvm-15/bin/llvm-config" CACHE FILEPATH "llvm-config used to build the engine")

foreach(probe
    "${ENGINE_TAICHI_ROOT}/taichi/program/program.h"
    "${ENGINE_TAICHI_ROOT}/taichi/ir/ir_builder.h"
    "${ENGINE_TAICHI_BUILD}/libtaichi_core_static.a"
    "${ENGINE_TAICHI_RUNTIME_SOURCE}/runtime_x64.bc"
    "${ENGINE_TAICHI_RUNTIME_SOURCE}/runtime_cuda.bc"
    "${ENGINE_TAICHI_ROOT}/external/cuda_libdevice/slim_libdevice.10.bc")
  if(NOT EXISTS "${probe}")
    message(FATAL_ERROR "Engine incomplete: ${probe} is missing. Build the Taichi fork first or set ENGINE_TAICHI_ROOT/ENGINE_TAICHI_BUILD.")
  endif()
endforeach()

# The engine splits its object code across per-component archives; the merged
# taichi_core archive does not contain them.
set(_engine_components
    taichi/common/libtaichi_common.a
    taichi/util/libtaichi_util.a
    taichi/compilation_manager/libcompilation_manager.a
    taichi/codegen/cpu/libcpu_codegen.a
    taichi/codegen/cuda/libcuda_codegen.a
    taichi/codegen/llvm/libllvm_codegen.a
    taichi/runtime/cpu/libcpu_runtime.a
    taichi/runtime/cuda/libcuda_runtime.a
    taichi/runtime/llvm/libllvm_runtime.a
    taichi/runtime/program_impls/llvm/libllvm_program_impl.a
    taichi/rhi/libti_device_api.a
    taichi/rhi/common/libcommon_rhi.a
    taichi/rhi/cpu/libcpu_rhi.a
    taichi/rhi/cuda/libcuda_rhi.a
    taichi/rhi/llvm/libllvm_rhi.a
    taichi/rhi/interop/libinterop_rhi.a)

set(_engine_libs "${ENGINE_TAICHI_BUILD}/libtaichi_core_static.a")
foreach(component ${_engine_components})
  if(NOT EXISTS "${ENGINE_TAICHI_BUILD}/${component}")
    message(FATAL_ERROR "Engine build tree is missing ${component}; rebuild the engine.")
  endif()
  list(APPEND _engine_libs "${ENGINE_TAICHI_BUILD}/${component}")
endforeach()

if(NOT EXISTS "${ENGINE_LLVM_CONFIG}")
  message(FATAL_ERROR "llvm-config not found at ${ENGINE_LLVM_CONFIG}")
endif()
execute_process(COMMAND "${ENGINE_LLVM_CONFIG}" --includedir
                OUTPUT_VARIABLE ENGINE_LLVM_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "${ENGINE_LLVM_CONFIG}" --libdir
                OUTPUT_VARIABLE ENGINE_LLVM_LIB_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "${ENGINE_LLVM_CONFIG}" --version
                OUTPUT_VARIABLE ENGINE_LLVM_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "${ENGINE_LLVM_CONFIG}" --libs
                OUTPUT_VARIABLE _engine_llvm_libs OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "${ENGINE_LLVM_CONFIG}" --system-libs
                OUTPUT_VARIABLE _engine_llvm_system_libs OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(_engine_llvm_libs UNIX_COMMAND "${_engine_llvm_libs}")
separate_arguments(_engine_llvm_system_libs UNIX_COMMAND "${_engine_llvm_system_libs}")

if(NOT ENGINE_LLVM_VERSION VERSION_GREATER_EQUAL 15)
  message(FATAL_ERROR "The engine is built against LLVM 15; ${ENGINE_LLVM_CONFIG} reports ${ENGINE_LLVM_VERSION}.")
endif()

execute_process(COMMAND git -C "${ENGINE_TAICHI_ROOT}" rev-parse HEAD
                OUTPUT_VARIABLE ENGINE_TAICHI_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET)
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

# These must match the engine's own compile definitions: they select the
# architecture, the LLVM backend and the CUDA backend inside engine headers.
target_compile_definitions(engine_taichi INTERFACE
    TI_INCLUDED TI_WITH_LLVM TI_WITH_CUDA TI_ARCH_x64 TI_ISE_NONE
    _GNU_SOURCE __STDC_CONSTANT_MACROS __STDC_FORMAT_MACROS __STDC_LIMIT_MACROS)

target_link_libraries(engine_taichi INTERFACE
    -Wl,--start-group ${_engine_libs} -Wl,--end-group
    "-L${ENGINE_LLVM_LIB_DIR}" ${_engine_llvm_libs} ${_engine_llvm_system_libs}
    Threads::Threads ${CMAKE_DL_LIBS})

# Stage the three files TI_LIB_DIR actually consumes without depending on
# Taichi's Python package/install layout.
set(ENGINE_TAICHI_RUNTIME_DIR "${CMAKE_CURRENT_BINARY_DIR}/taichi-runtime")
file(MAKE_DIRECTORY "${ENGINE_TAICHI_RUNTIME_DIR}")
configure_file("${ENGINE_TAICHI_RUNTIME_SOURCE}/runtime_x64.bc"
               "${ENGINE_TAICHI_RUNTIME_DIR}/runtime_x64.bc" COPYONLY)
configure_file("${ENGINE_TAICHI_RUNTIME_SOURCE}/runtime_cuda.bc"
               "${ENGINE_TAICHI_RUNTIME_DIR}/runtime_cuda.bc" COPYONLY)
configure_file("${ENGINE_TAICHI_ROOT}/external/cuda_libdevice/slim_libdevice.10.bc"
               "${ENGINE_TAICHI_RUNTIME_DIR}/slim_libdevice.10.bc" COPYONLY)

message(STATUS "Engine checkout : ${ENGINE_TAICHI_ROOT} (${ENGINE_TAICHI_REVISION})")
message(STATUS "Engine build    : ${ENGINE_TAICHI_BUILD}")
message(STATUS "Engine runtime source : ${ENGINE_TAICHI_RUNTIME_SOURCE}")
message(STATUS "Engine runtime stage  : ${ENGINE_TAICHI_RUNTIME_DIR}")
message(STATUS "Engine LLVM     : ${ENGINE_LLVM_VERSION} (${ENGINE_LLVM_LIB_DIR})")
