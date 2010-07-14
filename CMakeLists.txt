project(libdebconf-kde)

cmake_minimum_required(VERSION 2.6.0)

set(DEBCONF_VERSION 0.1.0)

set(DEBCONF_LIB_SOVERSION 0)
set(DEBCONF_LIB_VERSION ${DEBCONF_LIB_SOVERSION}.1.0)

find_package(KDE4 REQUIRED)

include(KDE4Defaults)

add_definitions(
    ${KDE_DEFINITIONS}
)

include_directories(
    ${KDE4_INCLUDES}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_subdirectory(cmake)
add_subdirectory(src)
add_subdirectory(tools)
