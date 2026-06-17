cmake_minimum_required(VERSION 3.16)
project(hcp_settle CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Standalone reference + test harness for the AZSL settle port (slice 1-2).
# SettleKernel.h is the portable CPU reference and the deterministic ORACLE the
# AZSL compute shader (HCPSettleCompute.azsl) is validated against. See README.md.
enable_testing()

# ---- CPU reference tests (always built; no GPU/O3DE needed) ----
add_executable(test_settle test_settle.cpp)
target_compile_options(test_settle PRIVATE -O2 -Wall -Wextra)
add_test(NAME test_settle COMMAND test_settle)

# ---- GPU equivalence harness (built only when O3DE toolchain is present) ----
# Proves the kernel's SPIR-V run on real Vulkan hardware == the CPU reference.
# Self-skips (exit 0) at run time if no GPU / no settle.spv.
set(O3DE_ROOT "/opt/O3DE/25.10.2" CACHE PATH "O3DE install (azslc/dxc + GLAD Vulkan header)")
file(GLOB_RECURSE _glad_vk "${O3DE_ROOT}/Gems/Atom/RHI/Vulkan/External/glad/*/include/glad/vulkan.h")

if(_glad_vk)
    list(GET _glad_vk 0 _glad_header)
    get_filename_component(_glad_dir "${_glad_header}" DIRECTORY)     # .../include/glad
    get_filename_component(GLAD_INCLUDE "${_glad_dir}" DIRECTORY)     # .../include

    # Build settle.spv via the portable chain (azslc -> HLSL -> dxc -> SPIR-V).
    set(_spv "${CMAKE_CURRENT_BINARY_DIR}/settle.spv")
    add_custom_command(
        OUTPUT "${_spv}"
        COMMAND O3DE_ROOT=${O3DE_ROOT} "${CMAKE_CURRENT_SOURCE_DIR}/build_spirv.sh" "${_spv}"
        DEPENDS HCPSettleCompute.azsl HCPSettleSrg.azsli build_spirv.sh
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Compiling HCPSettleCompute.azsl -> SPIR-V")
    add_custom_target(settle_spirv ALL DEPENDS "${_spv}")

    add_executable(vk_settle_harness vk_settle_harness.cpp)
    target_include_directories(vk_settle_harness PRIVATE "${GLAD_INCLUDE}")
    # GLAD's header trips field-initializer / unused warnings; quiet just those.
    target_compile_options(vk_settle_harness PRIVATE -O2 -Wno-missing-field-initializers)
    target_link_libraries(vk_settle_harness PRIVATE dl)
    add_dependencies(vk_settle_harness settle_spirv)

    add_test(NAME vk_settle_harness COMMAND vk_settle_harness "${_spv}")
    message(STATUS "hcp_settle: GPU equivalence harness ENABLED (GLAD: ${_glad_header})")
else()
    message(STATUS "hcp_settle: GPU harness disabled (O3DE GLAD Vulkan header not found under ${O3DE_ROOT})")
endif()
