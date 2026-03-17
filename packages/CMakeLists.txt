add_library(catch2 STATIC catch2/src/catch_amalgamated.cpp)

# Add include path
target_include_directories(catch2 INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)