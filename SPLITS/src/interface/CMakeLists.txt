cmake_minimum_required(VERSION 3.10)
include(GNUInstallDirs)

#set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find requirements
find_package(fastcdr REQUIRED)
find_package(fastrtps 2.12 REQUIRED)

# SOME/IP
find_package(CommonAPI 3.2.0 REQUIRED)
find_package(CommonAPI-SomeIP 3.2.0 REQUIRED)
find_package(vsomeip3 3.1.0 REQUIRED)

#find_library(${SPLITS_DDS_GEN_LIB} NAMES splitsdds_gen REQUIRED)
#message(STATUS "SPLITS_DDS_GEN_LIB: ${SPLITS_DDS_GEN_LIB}")

if (${BUILD_TARGET} STREQUAL "client" OR ${BUILD_TARGET} STREQUAL "client_on_target")
    set(TARGET splits_interface_client)
    set(TARGET_SOURCES
        data/SplitsDataPubSub.cpp
        data/SplitsDataSubscriber.cpp
        data/SplitsDataSubscriberSae.cpp
        data/SplitsDataSubscriberCsae.cpp
        data/SplitsDataPublisher.cpp
        data/SplitsClientDataPublisherSae.cpp
        data/SplitsClientDataPublisherCsae.cpp
    )
    add_library( ${TARGET} SHARED ${TARGET_SOURCES})
if (${BUILD_TARGET} STREQUAL "client")
    target_include_directories(${TARGET} PRIVATE
        ${SPLITS_ROOT_PATH}/
        ${SPLITS_ROOT_PATH}/include
        ${SPLITS_ROOT_PATH}/src/core
        ${CMAKE_INSTALL_PREFIX}/include/
        ${CMAKE_INSTALL_PREFIX}/include/splits-commonapi-gen/core
        ${CMAKE_INSTALL_PREFIX}/include/splits-commonapi-gen/someip
        ${CMAKE_INSTALL_PREFIX}/include/splits-dds-gen/
        ${COMMONAPI_INCLUDE_DIRS}
    )
else()
    target_include_directories(${TARGET} PRIVATE
        ${SPLITS_ROOT_PATH}/
        ${SPLITS_ROOT_PATH}/include
        ${SPLITS_ROOT_PATH}/src/core
        ${SYS_INCLUDE}/splits-commonapi-gen/core
        ${SYS_INCLUDE}/splits-commonapi-gen/someip
        ${SYS_INCLUDE}/splits-dds-gen
        ${COMMONAPI_INCLUDE_DIRS}
    )
endif()
else()
set(TARGET splits_interface)
    set(TARGET_SOURCES
        data/SplitsDataPubSub.cpp
        data/SplitsDataSubscriber.cpp
        data/SplitsDataSubscriberSae.cpp
        data/SplitsDataSubscriberCsae.cpp
        data/SplitsDataPublisher.cpp
        data/SplitsServiceDataPublisherSae.cpp
        data/SplitsServiceDataPublisherCsae.cpp
        stubs/SplitsStatisticsStubImpl.cpp
        vehicle/VehicleInterfaceLocal.cpp
        vehicle/VehicleInterfaceIdl.cpp
        location/KinematicsReceive.cpp
    )
    add_library( ${TARGET} SHARED ${TARGET_SOURCES})
    target_include_directories(${TARGET} PRIVATE
        ${SPLITS_ROOT_PATH}/
        ${SPLITS_ROOT_PATH}/include
        ${SPLITS_ROOT_PATH}/config
        ${SPLITS_ROOT_PATH}/src/core
        ${SPLITS_ROOT_PATH}/src/interface
        ${SPLITS_ROOT_PATH}/src/Utils
        ${SPLITS_ROOT_PATH}/commonapi/core
        ${SPLITS_ROOT_PATH}/src/radio/RadioInterface
        ${SPLITS_ROOT_PATH}/src/radio/RadioTransmit
        ${SPLITS_ROOT_PATH}/src/radio/RadioReceive
        ${SYS_INCLUDE}/splits-commonapi-gen/core
        ${SYS_INCLUDE}/splits-commonapi-gen/someip
        ${SYS_INCLUDE}/splits-dds-gen
        ${COMMONAPI_INCLUDE_DIRS}
    )
endif()
target_link_libraries(${TARGET} fastcdr fastrtps)

install(TARGETS ${TARGET}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
