
# Setup the gem meta variables that includes the "gem_path", "gem_name" and "gem_version"
# The "pal_dir" variable is also set which refers to the Platform/<platform-name> directory
# based on the directory of this CMakeLists.txt
o3de_gem_setup("HCPEngine")

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this project
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Now that we have loaded our project traits for this platform, see if this project is even supported on this platform.
# If its not supported we just return after including the unsupported.
if(NOT PAL_TRAIT_HCPENGINE_SUPPORTED)
    return()
endif()

# Find libpq for PostgreSQL database access
find_library(LIBPQ_LIBRARY NAMES pq PATHS /usr/lib/x86_64-linux-gnu)
find_path(LIBPQ_INCLUDE_DIR NAMES libpq-fe.h PATHS /usr/include/postgresql)

# Find liblmdb for vocabulary cache reads
find_library(LIBLMDB_LIBRARY NAMES lmdb PATHS /usr/lib/x86_64-linux-gnu)
find_path(LIBLMDB_INCLUDE_DIR NAMES lmdb.h PATHS /usr/include)

# RapidJSON for socket server JSON parsing (already installed as O3DE 3rdParty package)
set(RAPIDJSON_INCLUDE_DIR "$ENV{HOME}/.o3de/3rdParty/packages/RapidJSON-1.1.0-rev1-multiplatform/RapidJSON/include")

# We are on a supported platform, so add the ${gem_name} target
# Note: We include the common files and the platform specific files which are set in hcpengine_files.cmake and
# in ${pal_dir}/hcpengine_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake

# The ${gem_name}.Private.Object target is an internal target
# It should not be used outside of this CMakeLists.txt
ly_add_target(
    NAME ${gem_name}.Private.Object STATIC
    NAMESPACE Gem
    FILES_CMAKE
        hcpengine_files.cmake
        ${pal_dir}/hcpengine_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            ${LIBPQ_INCLUDE_DIR}
            ${LIBLMDB_INCLUDE_DIR}
            ${RAPIDJSON_INCLUDE_DIR}
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzGameFramework
            Gem::Atom_AtomBridge.Static
            Gem::PhysX5.Static
            ${LIBPQ_LIBRARY}
            ${LIBLMDB_LIBRARY}
)

ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        hcpengine_shared_files.cmake
        ${pal_dir}/hcpengine_shared_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::${gem_name}.Private.Object
            AZ::AzCore
)

# Include the gem name into the Client Module source file
# for use with the AZ_DECLARE_MODULE_CLASS macro
# This is to allow renaming of the gem to also cause
# the CreateModuleClass_Gem_<gem-name> function which
# is used to bootstrap the gem in monolithic builds to link to the new gem name
ly_add_source_properties(
SOURCES
    Source/HCPEngineModule.cpp
PROPERTY COMPILE_DEFINITIONS
    VALUES
        O3DE_GEM_NAME=${gem_name}
        O3DE_GEM_VERSION=${gem_version})

# Runtime aliases — in editor builds, Clients points to the Editor module so only
# one system component loads (the editor one, which inherits all runtime functionality).
ly_create_alias(NAME ${gem_name}.Servers  NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified  NAMESPACE Gem TARGETS Gem::${gem_name})

################################################################################
# Editor module — Qt panels for the O3DE Editor
################################################################################

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    # Editor private object library (Qt widgets, editor system component)
    ly_add_target(
        NAME ${gem_name}.Editor.Private.Object STATIC
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            hcpengine_editor_private_files.cmake
        TARGET_PROPERTIES
            O3DE_PRIVATE_TARGET TRUE
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
                ${LIBPQ_INCLUDE_DIR}
                ${LIBLMDB_INCLUDE_DIR}
                ${RAPIDJSON_INCLUDE_DIR}
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                Gem::${gem_name}.Private.Object
    )

    # Editor module (.so) — loaded by the O3DE Editor
    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            hcpengine_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::${gem_name}.Editor.Private.Object
                ${LIBPQ_LIBRARY}
                ${LIBLMDB_LIBRARY}
    )

    ly_add_source_properties(
        SOURCES
            Source/Tools/HCPEngineEditorModule.cpp
        PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

    ly_create_alias(NAME ${gem_name}.Clients  NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
else()
    # Non-host-tools builds: fall back to runtime module everywhere
    ly_create_alias(NAME ${gem_name}.Clients  NAMESPACE Gem TARGETS Gem::${gem_name})
    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name})
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name})
endif()

# Add in CMake dependencies for each gem dependency listed in this gem's gem.json file
# for the primary gem variants used in most o3de applications
o3de_add_variant_dependencies_for_gem_dependencies(GEM_NAME ${gem_name} VARIANTS Builders Tools Clients Servers Unified)

################################################################################
# Gem dependencies
################################################################################

# Query the project name from the nearest project.json file in a directory at or above
# the current one.
# This gem is the project gem and therefore should be part of the project that is using it
o3de_find_ancestor_project_root(project_path project_name "${CMAKE_CURRENT_SOURCE_DIR}")

# If the project name could not be queried from a project.json file, then fallback
# to using the name of the project provided when the project template was instantiated
if (NOT project_name)
    set(project_name HCPEngine)
endif()
