cmake_minimum_required(VERSION 3.10)
include(GNUInstallDirs)

set(TARGET_TOOL splits_statistics)

set(TARGET_TOOL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/SplitsStatistics.cpp
    )

find_package(CommonAPI 3.2.0 REQUIRED)
find_package(CommonAPI-SomeIP 3.2.0 REQUIRED)
find_package(vsomeip3 3.1.0 REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -pthread")
add_executable(${TARGET_TOOL} ${TARGET_TOOL_SOURCES})

if (${BUILD_TARGET} STREQUAL "client")
include_directories(BEFORE
    ${SPLITS_ROOT_PATH}
    ${SPLITS_ROOT_PATH}/include
    ${COMMONAPI_INCLUDE_DIRS}
    ${CMAKE_INSTALL_PREFIX}/include/splits-commonapi-gen/core
    ${CMAKE_INSTALL_PREFIX}/include/splits-commonapi-gen/someip
    )
else()
include_directories(BEFORE
    ${SPLITS_ROOT_PATH}
    ${SPLITS_ROOT_PATH}/include
    ${COMMONAPI_INCLUDE_DIRS}
    ${SYS_INCLUDE}/splits-commonapi-gen/core
    ${SYS_INCLUDE}/splits-commonapi-gen/someip
    )
target_link_libraries(${TARGET_TOOL} telux_common)

endif()

target_link_libraries(${TARGET_TOOL} CommonAPI)
install(TARGETS ${TARGET_TOOL}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR})
