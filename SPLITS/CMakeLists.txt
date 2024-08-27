cmake_minimum_required(VERSION 2.8)

project("Cv2xRemoteAPI")
set(PRJ_NAME Cv2xRemoteAPI)

#set(CMAKE_VERBOSE_MAKEFILE on)
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

#Fast-DDS and Fast-CDR
find_package(fastcdr REQUIRED)
find_package(fastrtps 2.12 REQUIRED)
message(STATUS "FASTRTPS_INCLUDE_DIRS: ${FASTRTPS_INCLUDE_DIR}")

# Boost
find_package( Boost 1.54 COMPONENTS system thread log REQUIRED )
include_directories( ${Boost_INCLUDE_DIR}
    ${SYS_INCLUDE}
    ${SYS_INCLUDE}/splits-dds-gen
    )

set(SPLITS_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(src)

