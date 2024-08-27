cmake_minimum_required(VERSION 3.10)
include(GNUInstallDirs)

set(TARGET_SERVICE splits_service)

set(TARGET_SERVICE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/SplitsService.cpp
    )

find_package(CommonAPI 3.2.0 REQUIRED)
find_package(CommonAPI-SomeIP 3.2.0 REQUIRED)
find_package(vsomeip3 3.1.0 REQUIRED)
find_package(jsoncpp REQUIRED)
find_library(SPLITSDDS_GEN_LIB NAMES splitsdds_gen REQUIRED)

include_directories(BEFORE 
    ${SPLITS_ROOT_PATH}
    ${SPLITS_ROOT_PATH}/include
    ${SPLITS_ROOT_PATH}/src/interface
    ${SPLITS_ROOT_PATH}/src/logger
    ${SPLITS_ROOT_PATH}/config
    ${SYS_INCLUDE}/splits-commonapi-gen/core
    ${SYS_INCLUDE}/splits-commonapi-gen/someip
    ${SYS_INCLUDE}/splits-dds-gen
    ${COMMONAPI_INCLUDE_DIRS}
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -pthread")

add_executable(${TARGET_SERVICE} ${TARGET_SERVICE_SOURCES})
target_link_libraries(${TARGET_SERVICE}
    splits_core splits_codec splits_interface ${SPLITSDDS_GEN_LIB}
    splits_logger v2x_veh CommonAPI v2x_msg_codec v2x_msg_codec_sae telux_common jsoncpp
)
install ( TARGETS ${TARGET_SERVICE}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
