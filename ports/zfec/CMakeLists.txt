cmake_minimum_required(VERSION 3.20)

project(zfec LANGUAGES C)

include(CMakePackageConfigHelpers)

add_library(zfec STATIC
    "${ZFEC_SOURCE_DIR}/fec.c"
)

target_include_directories(zfec
    PUBLIC
        "$<BUILD_INTERFACE:${ZFEC_SOURCE_DIR}>"
        "$<INSTALL_INTERFACE:include/zfec>"
)

install(
    TARGETS zfec
    EXPORT zfecTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    FILES "${ZFEC_SOURCE_DIR}/fec.h"
    DESTINATION include/zfec
)

install(
    EXPORT zfecTargets
    FILE zfecTargets.cmake
    NAMESPACE zfec::
    DESTINATION share/zfec
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/zfecConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/zfecConfig.cmake"
    INSTALL_DESTINATION share/zfec
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/zfecConfig.cmake"
    DESTINATION share/zfec
)