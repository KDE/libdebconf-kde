set(libdebconf_SRCS
    debconf.cpp
    DebconfElement.cpp
    DebconfBoolean.cpp
    DebconfError.cpp
    DebconfMultiselect.cpp
    DebconfNote.cpp
    DebconfPassword.cpp
    DebconfProgress.cpp
    DebconfSelect.cpp
    DebconfString.cpp
    DebconfText.cpp
    DebconfGui.cpp
    Debug.cpp)

ki18n_wrap_ui(libdebconf_SRCS
    DebconfBoolean.ui
    DebconfError.ui
    DebconfMultiselect.ui
    DebconfNote.ui
    DebconfPassword.ui
    DebconfProgress.ui
    DebconfSelect.ui
    DebconfString.ui
    DebconfText.ui
    DebconfGui.ui)

add_definitions(-DTRANSLATION_DOMAIN=\"libdebconf-kde\")
add_library(debconf-kde SHARED ${libdebconf_SRCS})
add_library(DebconfKDE::Main ALIAS debconf-kde)
target_link_libraries(debconf-kde
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Widgets
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Network
        KF${KF_MAJOR_VERSION}::CoreAddons
        KF${KF_MAJOR_VERSION}::I18n
        KF${KF_MAJOR_VERSION}::IconThemes
        KF${KF_MAJOR_VERSION}::TextWidgets
        KF${KF_MAJOR_VERSION}::WidgetsAddons
)
generate_export_header(debconf-kde)

target_include_directories(debconf-kde INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/DebconfKDE>")

set_target_properties(debconf-kde
    PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${DEBCONF_SOVERSION}
        EXPORT_NAME Main)

install(TARGETS debconf-kde EXPORT DebconfKDETargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES DebconfGui.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/DebconfKDE)
