cmake_minimum_required(VERSION 3.10)

project("Cv2xData")

set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find requirements
find_package(fastcdr REQUIRED)
find_package(fastrtps 2.12 REQUIRED)
find_library(CV2X_DATA_GEN_LIB NAMES Cv2xDataGen REQUIRED)

include_directories(AFTER ${TELUX_HEADER_PATH})
message(STATUS "Configuring Cv2xData...")
add_library(${CV2X_DATA_LIB} SHARED
        Cv2xDataPublisher.cpp
        Cv2xDataSubscriber.cpp
)
target_link_libraries(${CV2X_DATA_LIB} ${CV2X_DATA_GEN_LIB} fastcdr fastrtps)

install(TARGETS ${CV2X_DATA_LIB}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
