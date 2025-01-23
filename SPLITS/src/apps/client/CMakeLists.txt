cmake_minimum_required(VERSION 3.10)
include(GNUInstallDirs)

set(TARGET_CLIENT splits_client)

set(TARGET_CLIENT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/SplitsClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SplitsRemoteRTCM.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SplitsVehicleDataService.cpp
    ${SPLITS_ROOT_PATH}/config/SplitsConfigFile.cpp
    ${SPLITS_ROOT_PATH}/src/core/SplitsNtrip.cpp
    ${SPLITS_ROOT_PATH}/src/core/utils.c
    )
find_package(CommonAPI 3.2.0 REQUIRED)
find_package(CommonAPI-SomeIP 3.2.0 REQUIRED)
find_package(vsomeip3 3.1.0 REQUIRED)
find_package(jsoncpp REQUIRED)

include_directories(BEFORE 
    ${SPLITS_ROOT_PATH}
    ${SPLITS_ROOT_PATH}/include
    ${SPLITS_ROOT_PATH}/config
    ${SPLITS_ROOT_PATH}/src/interface
    ${SPLITS_ROOT_PATH}/src/core
    ${SPLITS_ROOT_PATH}/src/logger
    ${COMMONAPI_INCLUDE_DIRS}
    ${CMAKE_INSTALL_PREFIX}/include/cv2x-remote-gen/core
    ${CMAKE_INSTALL_PREFIX}/include
    ${CMAKE_INSTALL_PREFIX}/include/splits-dds-gen
    ${CMAKE_INSTALL_PREFIX}/include/splits-commonapi-gen/core
    ${CMAKE_INSTALL_PREFIX}/include/splits-commonapi-gen/someip
    )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
link_directories(${CMAKE_INSTALL_PREFIX}/lib)

add_executable(${TARGET_CLIENT} ${TARGET_CLIENT_SOURCES})

target_link_libraries(${TARGET_CLIENT}
    splits_interface CommonAPI splitsdds_gen splits_logger jsoncpp
)

install ( TARGETS ${TARGET_CLIENT}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR}
)
