cmake_minimum_required(VERSION 3.10)
set(CMAKE_FIND_DEBUG_MODE TRUE)

project("SPLITS")
include(GNUInstallDirs)

set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
#set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(SPLITS_DDS_LIB_GEN splitsdds_gen)

include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/SAE
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/CSAE
)

file(GLOB SPLITS_DDS_LIB_GEN_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/SAE/*.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/IEEE1609/*.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/CSAE/*.cxx
    )

# Find requirements
find_package(fastcdr REQUIRED)
find_package(fastrtps 2.12 CONFIG REQUIRED)
message(STATUS "rtps: ${FASTCDR_LIBRARIES}")

add_library(${SPLITS_DDS_LIB_GEN} SHARED ${SPLITS_DDS_LIB_GEN_SRCS})
target_link_libraries(${SPLITS_DDS_LIB_GEN} fastcdr fastrtps)
file(GLOB_RECURSE SPLITS_DDS_PUBLIC_HEADERS
    generated/SAE/*.h
    generated/IEEE1609/*.h
    generated/CSAE/*.h)

install(TARGETS ${SPLITS_DDS_LIB_GEN}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
install(FILES ${SPLITS_DDS_PUBLIC_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/splits-dds-gen/
    COMPONENT Headers
)

