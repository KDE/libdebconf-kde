project(libdebconf-kde)

cmake_minimum_required(VERSION 2.8.12)

find_package(ECM 0.0.14 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH
    ${ECM_MODULE_PATH}
    ${ECM_KDE_MODULE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

include(ECMPackageConfigHelpers)
include(ECMSetupVersion)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

set(REQUIRED_QT_VERSION 5.2.0) # Used in CMakeConfig to check deps
find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Widgets Network)

find_package(KF5CoreAddons 5.0.0 REQUIRED)
find_package(KF5I18n 5.0.0 REQUIRED)
find_package(KF5IconThemes 5.0.0 REQUIRED)
find_package(KF5TextWidgets 5.0.0 REQUIRED)

ecm_setup_version(1.0.0 VARIABLE_PREFIX DEBCONF
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/DebconfVersion.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/DebconfKDEConfigVersion.cmake"
    SOVERSION 1)

add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# create a Config.cmake and a ConfigVersion.cmake file and install them
set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/DebconfKDE")

ecm_configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/DebconfKDEConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/DebconfKDEConfig.cmake"
    INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR})

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/DebconfKDEConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/DebconfKDEConfigVersion.cmake"
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    COMPONENT Devel)
install(EXPORT DebconfKDETargets
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        FILE DebconfKDETargets.cmake
        NAMESPACE DebconfKDE::)

add_subdirectory(src)
add_subdirectory(tools)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
