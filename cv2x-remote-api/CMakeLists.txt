cmake_minimum_required(VERSION 2.8)

project("Cv2xRemoteAPI")
set(PRJ_NAME Cv2xRemoteAPI)

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
set(CMAKE_FIND_DEBUG_MODE TRUE)

OPTION(USE_FILE "Set to OFF to disable file logging" OFF )
message(STATUS "USE_FILE is set to value: ${USE_FILE}")

OPTION(USE_CONSOLE "Set to OFF to disable console logging" OFF )
message(STATUS "USE_CONSOLE is set to value: ${USE_CONSOLE}")

IF(USE_FILE)
  add_definitions(-DUSE_FILE)
ENDIF(USE_FILE)
IF(USE_CONSOLE)
  add_definitions(-DUSE_CONSOLE)
ENDIF(USE_CONSOLE)

SET(MAX_LOG_LEVEL "DEBUG" CACHE STRING "maximum log level")
message(STATUS "MAX_LOG_LEVEL is set to value: ${MAX_LOG_LEVEL}")
add_definitions(-DCOMMONAPI_LOGLEVEL=COMMONAPI_LOGLEVEL_${MAX_LOG_LEVEL})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread -O0 -std=c++11 -D_GLIBCXX_USE_NANOSLEEP")

message(STATUS "Compiler options: ${CMAKE_CXX_FLAGS}")

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
       "Choose the type of build, options are: Debug Release." FORCE)
endif(NOT CMAKE_BUILD_TYPE)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

OPTION(USE_INSTALLED_COMMONAPI "Set to OFF to use the local (build tree) version of CommonAPI" ON)
message(STATUS "USE_INSTALLED_COMMONAPI is set to value: ${USE_INSTALLED_COMMONAPI}")

if ("${USE_INSTALLED_COMMONAPI}" STREQUAL "ON")
        FIND_PACKAGE(CommonAPI 3.2.0 REQUIRED CONFIG NO_CMAKE_PACKAGE_REGISTRY)
else()
        FIND_PACKAGE(CommonAPI 3.2.0 REQUIRED CONFIG NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH)
endif()

message(STATUS "CommonAPI_CONSIDERED_CONFIGS: ${CommonAPI_CONSIDERED_CONFIGS}")
message(STATUS "COMMONAPI_INCLUDE_DIRS: ${COMMONAPI_INCLUDE_DIRS}")

# CommonAPI
include(FindPkgConfig)
###############################################################################

# SOME/IP
find_package (CommonAPI-SomeIP 3.2.0 REQUIRED)
find_package (vsomeip3 3.1.0 REQUIRED)

# Source Files
set(PRJ_SRC_PATH src)

#cv2x dds library
set(CV2X_DATA_LIB Cv2xData)
add_subdirectory(${PRJ_SRC_PATH}/dds)

#Service application name
set(PRJ_NAME_SERVICE ${PRJ_NAME}Service)
set(PRJ_SERVICE_SRCS
        ${PRJ_SRC_PATH}/${PRJ_NAME_SERVICE}.cpp
        ${PRJ_SRC_PATH}/Cv2xRadioManagerStubImpl.cpp
        ${PRJ_SRC_PATH}/Cv2xRadioStubImpl.cpp
    )
#client library name
set(PRJ_NAME_CLIENT telux_cv2x_remote)
set(CLIENT_WRAPPER_SRCS
        ${PRJ_SRC_PATH}/Utils.cpp
        ${PRJ_SRC_PATH}/Cv2xUtil.cpp
        ${PRJ_SRC_PATH}/CapiCv2xRadio.cpp
        ${PRJ_SRC_PATH}/CapiCv2xRadioManager.cpp
        ${PRJ_SRC_PATH}/CapiCv2xFactory.cpp
        ${PRJ_SRC_PATH}/Cv2xRxMetaDataHelper.cpp
    )

pkg_check_modules(GLIB REQUIRED glib-2.0)
add_compile_options(${GLIB_CFLAGS})


#Fast-DDS and Fast-CDR
find_package(fastcdr REQUIRED)
find_package(fastrtps 2.12 REQUIRED)

# Boost
find_package( Boost 1.54 COMPONENTS system thread log REQUIRED )
include_directories( ${Boost_INCLUDE_DIR} )

message(STATUS "TELUX_HEADER_PATH: ${TELUX_HEADER_PATH}")
include_directories(
    AFTER
    ${SYS_INCLUDE}/cv2x-remote-gen/core
    ${SYS_INCLUDE}/cv2x-remote-gen/someip
    src/dds
    ${COMMONAPI_INCLUDE_DIRS}
    ${VSOMEIP_INCLUDE_DIRS}
    ${TELUX_HEADER_PATH}
)

set(LINK_LIBRARIES -Wl,--as-needed CommonAPI)

if (${TARGET} STREQUAL "CLIENT")
    # Build Client
    message("Configuring for client")

    #need to include telSDK public header, the version should be the same as the cross-toolchain 
    include_directories(AFTER include)
    add_library (${PRJ_NAME_CLIENT} SHARED ${CLIENT_WRAPPER_SRCS})
    set_target_properties(${PRJ_NAME_CLIENT} PROPERTIES LINKER_LANGUAGE CXX)
    target_link_libraries(${PRJ_NAME_CLIENT} ${LINK_LIBRARIES} ${CV2X_DATA_LIB} fastcdr fastrtps)

    install(TARGETS ${PRJ_NAME_CLIENT}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} lib
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} lib
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} include
    )
    if ("${TELUX_SRC_PATH}" STREQUAL "")
        message(STATUS "telux_src not defined")
    else()
        message(STATUS "telux_src defined ${TELUX_SRC_PATH}")
        #Build legacy C telSDK APIs
        set(LIB_V2X_RADIO v2x_radio)
        set(LIB_V2X_RADIO_SRCS
            ${TELUX_SRC_PATH}/cv2x/legacy/radio_telux.cpp
            ${TELUX_SRC_PATH}/cv2x/legacy/v2x_packet.cpp
            ${TELUX_SRC_PATH}/cv2x/legacy/log.c
        )
        add_definitions(-DUSE_IVSS)
        add_library(${LIB_V2X_RADIO} SHARED ${LIB_V2X_RADIO_SRCS})
        set_target_properties(${LIB_V2X_RADIO} PROPERTIES LINKER_LANGUAGE CXX)
        target_link_libraries(${LIB_V2X_RADIO} ${PRJ_NAME_CLIENT})
        install(TARGETS ${LIB_V2X_RADIO} 
             ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} lib
             LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} lib
             PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} include
        )
    endif()

    #build "acme"
    if (DEFINED BUILD_ACME)
        if ("${TELUX_SRC_PATH}" STREQUAL "")
            message(STATUS "TELUX_SRC_PATH is not specified, not building acme")
        else()
            set(ACME_SRCS 
               ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/apps/src/acme.c
               ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/apps/src/tunnel_mode.c
               ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/apps/src/message_parse.c
            )
            add_definitions(-DUSE_IVSS)
            add_executable(acme ${ACME_SRCS})
            include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/apps/inc
                ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/inc)
            target_link_libraries(acme ${GLIB_LIBRARIES} ${LIB_V2X_RADIO} rt m)
            install(TARGETS acme
                  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} bin
            )
        endif()
    endif()
else()
    # Build service
    message("Configuring for service")
    add_executable(${PRJ_NAME_SERVICE} ${PRJ_SERVICE_SRCS})
    target_link_libraries(${PRJ_NAME_SERVICE} ${LINK_LIBRARIES} ${CV2X_DATA_LIB} telux_cv2x fastcdr fastrtps)

    install(TARGETS ${PRJ_NAME_SERVICE} 
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
