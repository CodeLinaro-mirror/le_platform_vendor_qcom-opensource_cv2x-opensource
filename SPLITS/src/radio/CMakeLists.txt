cmake_minimum_required(VERSION 3.10)

#set(CMAKE_VERBOSE_MAKEFILE ON)
include(GNUInstallDirs)

set(TARGET_RADIO splits_radio)

set(SPLITS_RADIO_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/./RadioInterface/RadioInterface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/./RadioReceive/RadioReceive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/./RadioTransmit/RadioTransmit.cpp
)
add_library(${TARGET_RADIO} SHARED ${SPLITS_RADIO_SOURCES})

target_include_directories(${TARGET_RADIO} PRIVATE
     ${CMAKE_CURRENT_SOURCE_DIR}/RadioInterface
     ${TELUX_HEADER_PATH}
)

install(TARGETS ${TARGET_RADIO}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
