# Buffer Tests
add_executable(buffer_test buffer_tests.cpp)
target_link_libraries(buffer_test PRIVATE shim catch2)
target_link_options(buffer_test PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
)

# Cli Tests
add_executable(cli_test cli_tests.cpp)
target_link_libraries(cli_test PRIVATE shim catch2)
target_link_options(cli_test PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
)

# Event Tests
add_executable(event_test event_tests.cpp)
target_link_libraries(event_test PRIVATE shim catch2)
target_link_options(event_test PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
)