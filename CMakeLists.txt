cmake_minimum_required(VERSION 3.16.0)
cmake_policy(SET CMP0072 NEW)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(BetrockPlusPlus VERSION 0.1.0)

# Build as server
option(BUILD_SERVER "Build as server" OFF)

find_package(libdeflate CONFIG REQUIRED)

if(NOT BUILD_SERVER)
    find_package(OpenGL REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(glm CONFIG REQUIRED)
    find_package(OpenAL CONFIG REQUIRED)
    #find_package(SDL3 CONFIG REQUIRED)
endif()

include_directories(
    "${PROJECT_SOURCE_DIR}/external/glad/include"
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/src/bpp_shared"
    "${PROJECT_SOURCE_DIR}/src/bpp_shared/enums"
    "${PROJECT_SOURCE_DIR}/src/bpp_shared/enums/network"
    "${PROJECT_SOURCE_DIR}/src/bpp_shared/world"
    "${PROJECT_SOURCE_DIR}/src/bpp_shared/world/noise"
    "${PROJECT_SOURCE_DIR}/src/bpp_shared/helpers/java"
    "${PROJECT_SOURCE_DIR}/src/bpp_shared/helpers"
    "${PROJECT_SOURCE_DIR}/src/bpp_shared/logger"
)

# Command-file wildcard
file(GLOB COMMAND_SOURCES
    "src/bpp_server/commands/implementation/command_*.cpp"
)

add_executable(
    BetrockPlusPlus
    src/main.cpp

    # External files
    external/glad/src/glad.c

    # Shared files
    
    src/bpp_shared/enums/biomes.cpp
    src/bpp_shared/enums/items.cpp
    src/bpp_shared/strings/labels.cpp
    src/bpp_shared/networking/network_stream.cpp
    src/bpp_shared/networking/server_manager.cpp
    src/bpp_server/commands/command.cpp
    src/bpp_server/commands/command_manager.cpp
    ${COMMAND_SOURCES}
    src/bpp_shared/world/world.cpp
    src/bpp_shared/world/lighter.cpp
    src/bpp_shared/world/storage/region.cpp
    src/bpp_shared/blocks/block_properties.cpp 

    ## Generation
    src/bpp_shared/world/noise/noise_generator.cpp
    src/bpp_shared/world/noise/noise_perlin.cpp
    src/bpp_shared/world/noise/noise_simplex.cpp
    src/bpp_shared/world/generator/chunk_gen.cpp
    src/bpp_shared/world/generator/tree_gen.cpp
    src/bpp_shared/world/generator/biome_gen.cpp
    src/bpp_shared/world/generator/cave_gen.cpp
    src/bpp_shared/world/generator/feature_gen.cpp

    ## Logger
    src/bpp_shared/logger/logger.cpp
    src/bpp_shared/logger/logstream.cpp
    src/bpp_shared/logger/style.cpp
    src/bpp_shared/config/config.cpp
    
    # Server-specific files
    src/bpp_server/server.cpp

    # Client-specific files
    src/bpp_client/client.cpp
    src/bpp_client/inputs.cpp
    src/bpp_client/renderers/world_renderer.cpp
    src/bpp_client/renderers/sub_chunk_mesher.cpp
)

if(BUILD_SERVER)
    target_compile_definitions(BetrockPlusPlus PRIVATE BUILD_SERVER)
else()
    target_compile_definitions(BetrockPlusPlus PRIVATE BUILD_CLIENT)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(BetrockPlusPlus PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:
            # Full debug info (GDB-specific)
            $<$<PLATFORM_ID:Linux>:-ggdb>
            # AddressSanitizer (Linux only)
            $<$<PLATFORM_ID:Linux>:-fsanitize=address>
            # General debug symbols (fallback for non-Linux)
            -g
        >
    )
    target_link_options(BetrockPlusPlus PRIVATE
        # Link AddressSanitizer runtime (Linux only)
        $<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<PLATFORM_ID:Linux>>:
            -fsanitize=address
        >
    )
endif()

# Add some extra warnings
target_compile_options(BetrockPlusPlus PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:
        -Wall
        -Wextra
        # Implicit conversions (int32_t -> float, etc.)
        #-Wconversion
        # Sign mismatches
        -Wsign-conversion
        # Variable shadowing
        -Wshadow
        # Unused variables, params, etc.
        -Wunused
        -Wuninitialized
        # Treat warnings as errors
        #-Werror
        # Common OOP pitfall
        -Wnon-virtual-dtor
        # C-style casts
        #-Wold-style-cast
        # Hides virtual functions
        -Woverloaded-virtual
        # Dereferencing null pointers
        -Wnull-dereference
        # Implicit float -> double conversions
        #-Wdouble-promotion
        # More printf/scanf format checks
        -Wformat=2
        # Warn if struct initialization is missing fields
        #-Wmissing-field-initializers
        # Multiple declarations of the same function/variable
        -Wredundant-decls
        # Comparing floating-point values for equality can be iffy
        -Wfloat-equal
        # Warn about code that will never be executed (Clang only)
        $<$<CXX_COMPILER_ID:Clang>:-Wunreachable-code>
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        # Enable most warnings / treat warnings as errors
        /W4
        # /WX
        # conversion, possible loss of data
        /w14242
        # conversion, possible loss of data (more cases)
        /w14254
        # member function does not override base
        /w14263
        # unsigned/negative mismatch
        /w14287
        # nonstandard extension used (optional strictness)
        /we4289
        # expression is always false/true
        /w14296
        # expression before comma has no effect
        /w14545
        # function call before comma missing argument
        /w14546
        # operator before comma has no effect
        /w14547
        # operator before comma has no effect
        /w14549
        # expression has no effect
        /w14555
        # pragma warning misuse
        /w14619
        # thread unsafe static init
        /w14640
        # conversion sign mismatch (closest to -Wsign-conversion)
        /w14826
        # wide string literal cast
        /w14905
        # string literal cast
        /w14906
        # illegal copy initialization
        /w14928
        # Strict standards conformance
        /permissive-
        # Suppress "nonstandard extension: nameless struct/union" (no equivalent, extension is intentional)
        /wd4201
        # Implicit conversions (no direct equiv to -Wconversion, C4244 is closest)
        /w14244
        # Sign mismatches (C4245: signed/unsigned mismatch)
        /w14245
        # Variable shadowing (C4456/C4457/C4458/C4459)
        /w14456
        /w14457
        /w14458
        /w14459
        # Unreachable code (C4702)
        /w14702
        # Missing return statement (C4715)
        /w14715
        # Destructor not virtual (C4265)
        /w14265
        # Hides virtual functions (C4264)
        /w14264
        # Uninitialized variables (C4700: uninitialized local variable used)
        /w14700
        # static analysis (catches null deref, uninitialized, etc.)
        /analyze
    >
)

# Link libraries
target_link_libraries(
    BetrockPlusPlus
    PRIVATE
    $<IF:$<TARGET_EXISTS:libdeflate::libdeflate_shared>,libdeflate::libdeflate_shared,libdeflate::libdeflate_static>
    #OpenGL::GL
    #glfw
    #glm::glm
    #OpenAL::OpenAL
    #SDL3::SDL3
    #Threads::Threads
)

# Ensure all files have copyright notice
file(GLOB_RECURSE SOURCE_FILES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.h"
)

set(REQUIRED_STRINGS
    "Copyright \\(c\\)"
    "SPDX-License-Identifier: GPL-3.0-only"
)

foreach(FILE ${SOURCE_FILES})
    file(READ "${FILE}" CONTENTS)

    foreach(PATTERN ${REQUIRED_STRINGS})
        string(REGEX MATCH "${PATTERN}" MATCH_RESULT "${CONTENTS}")

        if(NOT MATCH_RESULT)
            message(FATAL_ERROR
                "Missing required copyright/SPDX notice in: ${FILE}"
            )
        endif()
    endforeach()
endforeach()

# Detect CPU architecture
if(CMAKE_SYSTEM_PROCESSOR)
    set(ARCH_SUFFIX "${CMAKE_SYSTEM_PROCESSOR}")
else()
    set(ARCH_SUFFIX "unknown")
endif()

# Set up CPack for AppImage
set(CPACK_PACKAGE_NAME "BetrockPlusPlus")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Betrock++ - Minecraft Beta 1.7.3-compatible Server and Client")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

# Override the default package file name
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${ARCH_SUFFIX}")

include(CPack)