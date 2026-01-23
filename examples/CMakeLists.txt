# Buffer Example
add_executable(buffer buffer.cpp)
target_link_libraries(buffer PRIVATE shim)

# Event Example
add_executable(event event.cpp)
target_link_libraries(event PRIVATE shim)