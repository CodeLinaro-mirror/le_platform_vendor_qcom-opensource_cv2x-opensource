cmake_minimum_required(VERSION 3.10)
project("SPLITS")
include(GNUInstallDirs)

set(LOGGER_TARGET splits_logger)

include_directories(BEFORE 
    ${SPLITS_ROOT_PATH}/include/
    ${CMAKE_INSTALL_PREFIX}/include/splits-dds-gen
    ${SYS_INCLUDE}/splits-dds-gen
    ${CMAKE_INSTALL_PREFIX}/include
    )
set(LOGGER_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/v2xSaeMsgLogger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SaeBsmLogger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SaeMapLogger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SaeSpatLogger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SaeRtcmLogger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SaeSdsmLogger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SaeRsmLogger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SaeSrmLogger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/v2xCsaeMsgLogger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CsaeBsmLogger.cpp
)

add_library(${LOGGER_TARGET} SHARED ${LOGGER_SRCS})

install(TARGETS ${LOGGER_TARGET}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

