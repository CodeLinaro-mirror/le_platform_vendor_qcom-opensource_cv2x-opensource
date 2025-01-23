cmake_minimum_required(VERSION 3.10)
if (${BUILD_TARGET} STREQUAL "client")
    add_subdirectory(client)
else()
    add_subdirectory(service)
endif()
add_subdirectory(tool)

