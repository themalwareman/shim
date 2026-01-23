cmake_minimum_required(VERSION 4.1)
project(shim)

# Header-only library
add_library(shim INTERFACE)

# Add include path
target_include_directories(shim INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# C++ 20
target_compile_features(shim INTERFACE cxx_std_20)

# Options
option(SHIM_BUILD_TESTS "Build shim tests" ON)
option(SHIM_BUILD_EXAMPLES "Build shim examples" ON)

if (SHIM_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if (SHIM_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()