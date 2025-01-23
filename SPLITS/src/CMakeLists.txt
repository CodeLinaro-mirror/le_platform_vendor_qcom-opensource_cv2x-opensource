if (${BUILD_TARGET} STREQUAL "client")
    add_compile_definitions($<$<CONFIG:Debug>:COMMONAPI_LOG>)
    include_directories(BEFORE ${COMMONAPI_INCLUDE_DIRS}>)
else()
    add_compile_definitions($<$<CONFIG:Debug>:TELSDK_LOG TELUX_TECH_AREA=2>)
    add_subdirectory(core)
    add_subdirectory(codec)
    add_subdirectory(radio)
endif()

add_subdirectory(interface)
add_subdirectory(apps)
add_subdirectory(logger)

