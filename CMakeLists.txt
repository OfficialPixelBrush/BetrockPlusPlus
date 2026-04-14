cmake_minimum_required(VERSION 3.16.0)
cmake_policy(SET CMP0072 NEW)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(BetrockPlusPlus VERSION 0.1.0)

find_package(libdeflate CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenAL CONFIG REQUIRED)

include_directories(
    "${PROJECT_SOURCE_DIR}/external/glad/include"
    "${PROJECT_SOURCE_DIR}/src/bpp_shared"
    "${PROJECT_SOURCE_DIR}/src/bpp_shared/enums"
    "${PROJECT_SOURCE_DIR}/src/bpp_shared/enums/network"
)

add_executable(
    BetrockPlusPlus

    external/glad/src/glad.c

    src/main.cpp

    src/bpp_shared/shared.cpp
    src/bpp_shared/enums/biomes.cpp
    src/bpp_shared/enums/drops.cpp
    src/bpp_shared/strings/labels.cpp
    src/bpp_shared/networking/network_stream.cpp
    
    src/bpp_server/server.cpp

    src/bpp_client/client.cpp
    src/bpp_client/inputs.cpp
    src/bpp_client/renderers/world_renderer.cpp
    src/bpp_client/renderers/sub_chunk_mesher.cpp
    src/bpp_shared/world/World.cpp
)

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

# If we're being extra pedantic
target_compile_options(BetrockPlusPlus PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:
        -Wall
        -Wextra
        # Implicit conversions (int32_t -> float, etc.)
        -Wconversion
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
        -Wold-style-cast
        # Hides virtual functions
        -Woverloaded-virtual
        # Dereferencing null pointers
        -Wnull-dereference
        # Implicit float -> double conversions
        -Wdouble-promotion
        # More printf/scanf format checks
        -Wformat=2
        # Warn if struct initialization is missing fields
        -Wmissing-field-initializers
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
    >
)

target_link_libraries(
    BetrockPlusPlus
    PRIVATE
    $<IF:$<TARGET_EXISTS:libdeflate::libdeflate_shared>,libdeflate::libdeflate_shared,libdeflate::libdeflate_static>
    OpenGL::GL
    glfw
    glm::glm
    OpenAL::OpenAL
    #${LUA_LIBRARIES}
    #Threads::Threads
)