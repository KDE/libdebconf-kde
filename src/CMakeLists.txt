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
    DebconfGui.cpp)

qt5_wrap_ui(libdebconf_SRCS
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

add_library(debconf-kde SHARED ${libdebconf_SRCS})
add_library(DebconfKDE ALIAS debconf-kde)
target_link_libraries(debconf-kde
    Qt5::Widgets
    Qt5::Network
    KF5::I18n
    KF5::IconThemes
    KF5::TextWidgets)

set_target_properties(debconf-kde
    PROPERTIES
        VERSION ${DEBCONF_VERSION_STRING}
        SOVERSION ${DEBCONF_SOVERSION}
        EXPORT_NAME DebconfKDE)

install(TARGETS debconf-kde EXPORT DebconfKDETargets ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES DebconfGui.h
    DESTINATION ${INCLUDE_INSTALL_DIR})
