project(cv2x-opensource)
include(FindPkgConfig)

# Glib for strlcpy and strlcat
pkg_check_modules(GLIB REQUIRED glib-2.0)
add_compile_options(${GLIB_CFLAGS})

include_directories(BEFORE
                    ${CMAKE_SOURCE_DIR}/inc)

# Build cv2x_apps_proxy
add_executable(cv2x_apps_proxy src/cv2x_apps_common.c src/cv2x_apps_proxy.c)
target_link_libraries(cv2x_apps_proxy v2x_radio ${GLIB_LIBRARIES})


# Install executables
install(TARGETS cv2x_apps_proxy
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
