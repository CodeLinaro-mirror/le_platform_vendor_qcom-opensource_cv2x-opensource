cmake_minimum_required(VERSION 3.10)
include(GNUInstallDirs)

set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
#set(CMAKE_VERBOSE_MAKEFILE on)

find_package(fastcdr REQUIRED)
find_package(fastrtps 2.12 REQUIRED)

# SOME/IP
#find_package(CommonAPI 3.2.0 REQUIRED)
find_package(CommonAPI 3.2.0 REQUIRED NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH)
find_package(CommonAPI-SomeIP 3.2.0 REQUIRED)
find_package(vsomeip3 3.1.0 REQUIRED)
find_library(SPLITSSRVICE_SOMIP NAMES SplitsService-someip REQUIRED)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")

set(SPLITS_CORE_TARGET splits_core)

set(SPLITS_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/SplitsCoreImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SplitsCoreFactoryImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BsmMsgGenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CongestCtrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TxScheduler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qUtils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CsvIO.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SecurityInterface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SplitsFilter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SplitsWra.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SplitsRTCM.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SplitsNtrip.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SplitsCoreDataListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils.c
    ${SPLITS_ROOT_PATH}/config/SplitsConfigFile.cpp
    ${SPLITS_ROOT_PATH}/config/SplitsConfigIdl.cpp
    )
add_library(${SPLITS_CORE_TARGET} SHARED ${SPLITS_CORE_SOURCES})

target_include_directories(${SPLITS_CORE_TARGET} PRIVATE
    ${SPLITS_ROOT_PATH}/include
    ${SPLITS_ROOT_PATH}/src/core
    ${SPLITS_ROOT_PATH}/src/codec
    ${SPLITS_ROOT_PATH}/src/interface
    ${SPLITS_ROOT_PATH}/src/radio/RadioInterface
    ${SPLITS_ROOT_PATH}/src/radio/RadioTransmit
    ${SPLITS_ROOT_PATH}/src/radio/RadioReceive
    ${SPLITS_ROOT_PATH}/config
    ${COMMONAPI_INCLUDE_DIRS}
    ${SYS_INCLUDE}/splits-commonapi-gen/core
    ${SYS_INCLUDE}/splits-commonapi-gen/someip
    ${SYS_INCLUDE}/splits-dds-gen
)
target_link_libraries(${SPLITS_CORE_TARGET} fastcdr fastrtps splits_radio
    telux_cv2x telux_loc telux_squish telux_sec viicsec pthread rt ${SPLITSSRVICE_SOMIP})

install(TARGETS ${SPLITS_CORE_TARGET}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
