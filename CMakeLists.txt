cmake_minimum_required(VERSION 3.16.0)
cmake_policy(SET CMP0072 NEW)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(BetrockPlusPlus VERSION 0.1.0)

include_directories(
    "${PROJECT_SOURCE_DIR}/src/bpp_shared"
)

add_executable(
    BetrockPlusPlus

    src/main.cpp
    src/bpp_server/server.cpp
    src/bpp_client/client.cpp
    src/bpp_shared/shared.cpp
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(UNIX AND NOT APPLE)
        target_compile_options(BetrockPlusPlus PRIVATE -ggdb)
        target_compile_options(BetrockPlusPlus PRIVATE -fsanitize=address)
        target_link_options(BetrockPlusPlus PRIVATE -fsanitize=address)
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

# If we're being extra pedantic
target_compile_options(BetrockPlusPlus PRIVATE
    # Make code more secure
    -Wall
    -Wextra
    # Can't use pedantic, otherwise it'll complain about anonymous structs/unions
    #-Wpedantic           # Enforce strict ISO compliance
    -Wconversion          # Implicit conversions (int32_t → float, etc.)
    -Wsign-conversion     # Sign mismatches
    -Wshadow              # Variable shadowing
    -Wunused              # Unused variables, params, etc.
    -Wuninitialized
    -Werror               # Treat warnings as errors
    -Wnon-virtual-dtor    # Common OOP pitfall
    -Wold-style-cast      # C-style casts
    -Woverloaded-virtual  # Hides virtual functions
    -Wnull-dereference    # Dereferencing null pointers
    -Wdouble-promotion    # Implicit float → double conversions
    -Wformat=2            # More printf/scanf format checks
    -Wmissing-field-initializers # Warn if struct initialization is missing fields
    -Wredundant-decls     # Multiple declarations of the same function/variable
    -Wfloat-equal         # Comparing floating-point values for equality can be iffy
    -Wunreachable-code    # Warn about code that will never be executed
)

target_link_libraries(
    BetrockPlusPlus
    #deflate
    #${LUA_LIBRARIES}
    #Threads::Threads
)