cmake_minimum_required(VERSION 3.10)
project("SPLITS")
include(GNUInstallDirs)

set(CODEC_TARGET splits_codec)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare -Wno-unused-variable")

include_directories(BEFORE 
    ${SPLITS_ROOT_PATH}/include
    ${SPLITS_ROOT_PATH}/src/core
    ${CMAKE_INSTALL_PREFIX}/include
    ${SYS_INCLUDE}
    ${SYS_INCLUDE}/splits-dds-gen
)
set(CODEC_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/SplitsCodecImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SplitsCodecFactory.cpp
)

add_library(${CODEC_TARGET} SHARED ${CODEC_SRCS})

target_link_libraries(${CODEC_TARGET} splits_sae_codec)

install(TARGETS ${CODEC_TARGET}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

