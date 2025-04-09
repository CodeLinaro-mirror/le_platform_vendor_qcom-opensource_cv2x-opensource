cmake_minimum_required(VERSION 3.10)
if (${BUILD_TARGET} STREQUAL "client" OR ${BUILD_TARGET} STREQUAL "client_on_target")
    add_subdirectory(client)
else()
    add_subdirectory(service)
endif()
add_subdirectory(tool)

