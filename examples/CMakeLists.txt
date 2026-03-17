# Buffer Example
add_executable(buffer buffer.cpp)
target_link_libraries(buffer PRIVATE shim)

# Event Example
add_executable(event event.cpp)
target_link_libraries(event PRIVATE shim)

# Synchronized Example
add_executable(synchronized synchronized.cpp)
target_link_libraries(synchronized PRIVATE shim)

# Cli Example
add_executable(cli cli.cpp)
target_link_libraries(cli PRIVATE shim)
target_link_options(cli PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
)